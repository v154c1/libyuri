/*!
 * @file 		GL.cpp
 * @author 		Zdenek Travnicek
 * @date 		22.10.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "GL.h"
#include "yuri/core/pipe/Pipe.h"
#include "yuri/core/thread/IOThread.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/utils.h"
#include "default_shaders.h"
#include <cassert>

namespace yuri {

namespace gl {
	mutex GL::big_gpu_lock;
namespace {

using namespace core;
/*!
 * Returns texture configuration for specified RGB format
 * @param fmt input format
 * @return A tuple containing internal format, input format and internal data type suitable for video frames in @fmt format.
 */
std::tuple<GLenum, GLenum, GLenum> get_rgb_format(const format_t fmt)
{
	switch (fmt) {
		case raw_format::rgb24:
			return std::make_tuple(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
		case raw_format::rgba32:
			return std::make_tuple(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
		case raw_format::bgr24:
			return std::make_tuple(GL_BGR, GL_RGB, GL_UNSIGNED_BYTE);
		case raw_format::bgra32:
			return std::make_tuple(GL_BGRA, GL_RGBA, GL_UNSIGNED_BYTE);
		case raw_format::r8:
		case raw_format::g8:
		case raw_format::b8:
		case raw_format::y8:
		case raw_format::u8:
		case raw_format::v8:
		case raw_format::depth8:
			return std::make_tuple(GL_LUMINANCE, GL_LUMINANCE8, GL_UNSIGNED_BYTE);
		case raw_format::r16:
		case raw_format::g16:
		case raw_format::b16:
		case raw_format::y16:
		case raw_format::u16:
		case raw_format::v16:
		case raw_format::depth16:
			return std::make_tuple(GL_LUMINANCE, GL_LUMINANCE16, GL_UNSIGNED_SHORT);
		case raw_format::rgb8:
			return std::make_tuple(GL_RGB, GL_R3_G3_B2,  GL_UNSIGNED_BYTE_3_3_2);
		case raw_format::rgb16:
			return std::make_tuple(GL_RGB, GL_RGB5,  GL_UNSIGNED_SHORT_5_6_5);
	    case raw_format::rgb_r10k_le:
            return std::make_tuple(GL_RGBA, GL_RGBA,  GL_UNSIGNED_INT_10_10_10_2);

	}
	return std::make_tuple(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
}

/*!
 * Returns correct shader for an yuv type.
 * @param fmt input format
 * @return copy of the shader string
 */
std::string get_yuv_shader(const format_t fmt)
{
	switch (fmt) {
		case raw_format::yuv444:
		case raw_format::yuva4444:
			return shaders::fs_get_yuv444;
		case raw_format::yuyv422:
			return shaders::fs_get_yuyv422;
		case raw_format::yvyu422:
			return shaders::fs_get_yvyu422;
		case raw_format::uyvy422:
			return shaders::fs_get_uyvy422;
		case raw_format::vyuy422:
			return shaders::fs_get_vyuy422;

	}
	return {};
}

size_t get_pixel_size(GLenum fmt, GLenum data_type)
{
	size_t comp_size = 0;
	switch (data_type) {
		case GL_UNSIGNED_BYTE_3_3_2:
			return 1;
		case GL_UNSIGNED_SHORT_5_6_5:
			return 2;
		case GL_UNSIGNED_SHORT_5_6_5_REV:
			return 2;
		case GL_UNSIGNED_BYTE:
			comp_size = 1;
			break;
		case GL_UNSIGNED_SHORT:
			comp_size = 2;
			break;
	    case GL_UNSIGNED_INT_2_10_10_10_REV:
	    case GL_UNSIGNED_INT_10_10_10_2:
	        return 4;
	}

	switch (fmt) {
		case GL_RGB:
		case GL_BGR:
			return 3*comp_size;
		case GL_RGBA:
		case GL_BGRA:
			return 3*comp_size;
		case GL_LUMINANCE:
			return comp_size;
		case GL_LUMINANCE_ALPHA:
			return 2*comp_size;
	}
	return 0;
}

const std::vector<format_t> gl_supported_formats = {
		raw_format::rgb24,
		raw_format::rgba32,
		raw_format::bgr24,
		raw_format::bgra32,
		raw_format::yuv444,
		raw_format::yuva4444,
		raw_format::yuyv422,
		raw_format::yvyu422,
		raw_format::uyvy422,
		raw_format::vyuy422,
		raw_format::yuv422p,
		raw_format::yuv444p,
		raw_format::yuv420p,
		raw_format::yuv411p,
        raw_format::nv12,
		raw_format::r8,
		raw_format::g8,
		raw_format::b8,
		raw_format::y8,
		raw_format::u8,
		raw_format::v8,
		raw_format::depth8,
		raw_format::r16,
		raw_format::g16,
		raw_format::b16,
		raw_format::y16,
		raw_format::u16,
		raw_format::v16,
		raw_format::depth16,
		raw_format::rgb8,
		raw_format::rgb16,
        raw_format::rgb_r10k_le,
};

}

std::vector<format_t> GL::get_supported_formats()
{
	return gl_supported_formats;
}

GL::GL(log::Log &log_):log(log_),
		shader_version_(120),
		corners{{-1.0f, -1.0f,
				1.0f, -1.0f,
				1.0f, 1.0f,
				-1.0f, 1.0f}},
		use_pbo(false)
{
	log.set_label("[GL] ");
}

GL::~GL() noexcept {

}

void GL::generate_texture(index_t tid, const core::pFrame& gframe, bool flip_x, bool flip_y)
{
	core::pRawVideoFrame frame = std::dynamic_pointer_cast<RawVideoFrame>(gframe);
	if (!frame) return;
	const format_t frame_format = frame->get_format();
	const resolution_t res = frame->get_resolution();
	generate_texture(tid, frame_format, res, gframe, flip_x, flip_y);
}

void GL::generate_texture(index_t tid, const format_t frame_format, const resolution_t res, const core::pFrame& gframe, bool flip_x, bool flip_y, bool force_pow2)
{
	using namespace yuri::core;
	core::pRawVideoFrame frame = std::dynamic_pointer_cast<RawVideoFrame>(gframe);
	std::string fs_color_get;

	textures[tid].flip_x = flip_x;
	textures[tid].flip_y = flip_y;

	auto &tx = textures[tid].tx1;
	auto &ty = textures[tid].ty1;

	const yuri::size_t w = res.width;
	const yuri::size_t h = res.height;

	const auto tex_res = resolution_t{force_pow2?next_power_2(w):w, force_pow2?next_power_2(h):h};
//	log[log::info] << "Textuire res: " << tex_res;
	tx = static_cast<float>(w) / static_cast<float>(tex_res.width);
	ty = static_cast<float>(h) / static_cast<float>(tex_res.height);


	if (textures[tid].tid[0]==static_cast<GLuint>(-1)) {
		textures[tid].gen_texture(0);
		log[log::info] << "Generated texture " << textures[tid].tid[0];
	}

	if (use_pbo && (textures[tid].pbo[0] == static_cast<GLuint>(-1))) {
		textures[tid].gen_buffer(0);
		log[log::info] << "Generated PBO " << textures[tid].pbo[0];
	}

	GLuint &tex = textures[tid].tid[0];

	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
	glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
	glPixelStorei(GL_PACK_LSB_FIRST, GL_FALSE);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);


	glBindTexture(GL_TEXTURE_2D, tex);
	glEnable(GL_MULTISAMPLE);
	glSampleCoverage(0.1f, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	const raw_format::raw_format_t& fi = raw_format::get_format_info(frame_format);
	switch (frame_format) {
		case raw_format::rgb24:
		case raw_format::rgba32:
		case raw_format::bgr24:
		case raw_format::bgra32:
		case raw_format::r8:
		case raw_format::g8:
		case raw_format::b8:
		case raw_format::y8:
		case raw_format::u8:
		case raw_format::v8:
		case raw_format::depth8:
		case raw_format::r16:
		case raw_format::g16:
		case raw_format::b16:
		case raw_format::y16:
		case raw_format::u16:
		case raw_format::v16:
		case raw_format::depth16:
		case raw_format::rgb8:
		case raw_format::rgb16:
		case raw_format::rgb_r10k_le:
		{
			GLenum fmt_in = GL_RGB;
			GLenum fmt_out = GL_RGB;
			GLenum data_type = GL_UNSIGNED_BYTE;
			std::tie(fmt_in, fmt_out, data_type) = get_rgb_format(frame_format);
			if (tex_res != textures[tid].tex_res) {
				prepare_texture(tid, 0, nullptr, 0, tex_res,fmt_out, fmt_in,false, data_type);
				textures[tid].tex_res = tex_res;
			}
			if (frame) {
				prepare_texture(tid, 0, PLANE_RAW_DATA(frame,0), PLANE_SIZE(frame,0), {w, h}, fmt_out, fmt_in, true, data_type);
			}
			fs_color_get = shaders::fs_get_rgb;
		}break;

		case raw_format::yuv444:
		case raw_format::yuva4444:
		case raw_format::yuyv422:
		case raw_format::yvyu422:
		case raw_format::uyvy422:
		case raw_format::vyuy422:
		{
			if (tex_res != textures[tid].tex_res) {
				if (frame_format==raw_format::yuv444) {
					prepare_texture(tid, 0, nullptr, 0, tex_res, GL_RGB, GL_RGB, false);
				} else if (frame_format==raw_format::yuva4444) {
					prepare_texture(tid, 0, nullptr, 0, tex_res, GL_RGB, GL_RGBA, false);
				} else {
					prepare_texture(tid, 0, nullptr, 0, {tex_res.width/2, tex_res.height}, GL_RGBA, GL_RGBA, false);
					prepare_texture(tid, 1, nullptr, 0, tex_res, GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, false);
				}
				textures[tid].tex_res =tex_res;
			}
			if (frame) {
				if (frame_format ==raw_format::yuv444) {
					prepare_texture(tid, 0, PLANE_RAW_DATA(frame,0), PLANE_SIZE(frame,0), {w, h},GL_RGB,GL_RGB,true);
				} else if (frame_format ==raw_format::yuva4444) {
					prepare_texture(tid, 0, PLANE_RAW_DATA(frame,0), PLANE_SIZE(frame,0), {w, h},GL_RGBA,GL_RGBA,true);
				} else {
					prepare_texture(tid, 0, PLANE_RAW_DATA(frame,0), PLANE_SIZE(frame,0), {w/2, h}, GL_RGBA, GL_RGBA, true);
					prepare_texture(tid, 1, PLANE_RAW_DATA(frame,0), PLANE_SIZE(frame,0), {w, h}, GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, true);
				}
			}
			fs_color_get = get_yuv_shader(frame_format);
		}break;


		case raw_format::yuv420p:
		case raw_format::yuv411p:
		case raw_format::yuv422p:
		case raw_format::yuv444p:{
			if (tex_res != textures[tid].tex_res) {
				for (int i=0;i<3;++i) {
					prepare_texture(tid,i,nullptr, 0, {tex_res.width/fi.planes[i].sub_x, tex_res.height/fi.planes[i].sub_y} ,GL_LUMINANCE8,GL_LUMINANCE,false);
				}
				textures[tid].tex_res = tex_res;
			}
			if (frame) {
				for (int i=0;i<3;++i) {
					prepare_texture(tid,i,PLANE_RAW_DATA(frame,i), PLANE_SIZE(frame,i),{w/fi.planes[i].sub_x,
							h/fi.planes[i].sub_y},GL_LUMINANCE8,GL_LUMINANCE,true);
				}
			}
			fs_color_get = shaders::fs_get_yuv_planar;

		}break;
        case raw_format::nv12:{
            if (tex_res != textures[tid].tex_res) {
                prepare_texture(tid,0,nullptr, 0, {tex_res.width/fi.planes[0].sub_x, tex_res.height/fi.planes[0].sub_y} ,GL_LUMINANCE8,GL_LUMINANCE,false);
                prepare_texture(tid,1,nullptr, 0, {tex_res.width/fi.planes[1].sub_x, tex_res.height/fi.planes[1].sub_y} ,GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA,false);

                textures[tid].tex_res = tex_res;
            }
            if (frame) {
                prepare_texture(tid,0,PLANE_RAW_DATA(frame,0), PLANE_SIZE(frame,0),{w/fi.planes[0].sub_x,
                                                                                    h/fi.planes[0].sub_y},GL_LUMINANCE8,GL_LUMINANCE,true);
                prepare_texture(tid,1,PLANE_RAW_DATA(frame,1), PLANE_SIZE(frame,1),{w/fi.planes[1].sub_x,
                                                                                    h/fi.planes[1].sub_y},GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA,true);
            }
            fs_color_get = shaders::fs_get_nv12;

        }break;

/*
		case YURI_FMT_DXT1:
		case YURI_FMT_DXT1_WITH_MIPMAPS:
//		case YURI_FMT_DXT2:
//		case YURI_FMT_DXT2_WITH_MIPMAPS:
		case YURI_FMT_DXT3:
		case YURI_FMT_DXT3_WITH_MIPMAPS:
//		case YURI_FMT_DXT4:
//		case YURI_FMT_DXT4_WITH_MIPMAPS:
		case YURI_FMT_DXT5:
		case YURI_FMT_DXT5_WITH_MIPMAPS:{
			GLenum format;
			yuri::format_t yf = frame->get_format();
			yuri::size_t fsize;
			if (yf==YURI_FMT_DXT1 || yf==YURI_FMT_DXT1_WITH_MIPMAPS) {
				format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				fsize = wh * wh >>1;
			} else if (yf==YURI_FMT_DXT3 || yf==YURI_FMT_DXT3_WITH_MIPMAPS) {
				format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				fsize = wh * wh;
			} else if (yf==YURI_FMT_DXT5 || yf==YURI_FMT_DXT5_WITH_MIPMAPS) {
				format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				fsize = wh * wh;
			} else break;
			bool mipmaps = true;
			if ((yf==YURI_FMT_DXT5) || (yf==YURI_FMT_DXT3) || (yf==YURI_FMT_DXT1))
				mipmaps = false;
			if (textures[tid].wh != wh) {
				yuri::size_t remaining=fsize, wh2=wh,next_level = fsize, offset = 0, level =0;
				char *image = new char[fsize];
				while (next_level <= remaining) {
					glCompressedTexImage2D(GL_TEXTURE_2D, level++, format, wh2, wh2, 0, next_level,image);
					if (!mipmaps) break;
					wh2>>=1;
					if (remaining<next_level) break;
					remaining-=next_level;
					offset+=next_level;
					next_level>>=2;
				}

				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//				GLenum e = glGetError();
//				log[log::error] << "compressed texture, " << e;
				delete[] image;
				textures[tid].wh = wh;
			}
			glBindTexture(GL_TEXTURE_2D, tex);
			fsize=w*h>>((yf==YURI_FMT_DXT1)?1:0);

			yuri::size_t remaining=PLANE_SIZE(frame,0), w2=w, h2=h, next_level = fsize, offset = 0, level =0;
			while (next_level <= remaining) {
				log[log::debug] << "next_level: " << next_level << ", rem: " << remaining;
				glCompressedTexSubImage2D(GL_TEXTURE_2D, level++, 0, 0, w2, h2,	format, next_level, PLANE_RAW_DATA(frame,0)+offset);
				if (!mipmaps) break;
				w2>>=1;h2>>=1;
				if (remaining<next_level) break;
				remaining-=next_level;
				offset+=next_level;
				next_level>>=2;
			}
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL, level-1);
			//GLenum e =
			glGetError();
			textures[tid].finish_update(log,frame->get_format(),simple_vertex_shader,
								simple_fragment_shader);
		} break;

		*/
		default:
			log[log::warning] << "Frame with unsupported format! (" << fi.name << ")";
			break;
	}
	/*
	log[log::debug] << "Generated texture " << wh << "x" << wh << " from image " <<
			w << "x" << h << " (" << tx << ", " << ty << ")";
	 */

	if (!fs_color_get.empty()) {
		if (textures[tid].shader_update_needed(frame_format)) {
			textures[tid].finish_update(log, frame_format,
								shaders::prepare_vs(shader_version_),
								shaders::prepare_fs(fs_color_get, transform_shader, color_map_shader, shader_version_));
		}
	}

	glPopClientAttrib();
}

void GL::generate_empty_texture(index_t tid, yuri::format_t fmt, resolution_t resolution)
{
	core::pFrame dummy = core::RawVideoFrame::create_empty(fmt,resolution);
	generate_texture(tid,dummy);
}

void GL::use_remote_texture(index_t tid, GLuint name, const format_t frame_format, const resolution_t res, bool flip_x, bool flip_y, float tx0, float ty0, float tx1, float ty1)
{
	textures[tid].tid[0] = name;
	generate_texture(tid, frame_format, res, {}, flip_x, flip_y, false);
	textures[tid].tx0 = tx0;
	textures[tid].tx1 = tx1;
	textures[tid].ty0 = ty0;
	textures[tid].ty1 = ty1;

}

void GL::setup_ortho(GLdouble left, GLdouble right,	GLdouble bottom,
		GLdouble top, GLdouble near, GLdouble far)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left, right, bottom, top, near, far);
	glClearColor(0,0,0,0);
	//glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void GL::draw_texture(index_t tid)
{
	auto& texture = textures[tid];
	GLuint &tex = texture.tid[0];
	if (tex==(GLuint)-1) return;
	if (!texture.shader) return;

//	bool &keep_aspect = textures[tid].keep_aspect;

	glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_POLYGON_BIT|GL_SCISSOR_BIT|GL_TEXTURE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_TEXTURE_2D);

//	GLdouble &dx = textures[tid].dx;
//	GLdouble &dy = textures[tid].dy;
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	texture.shader->use();
	texture.bind_texture_units();

	glActiveTexture(GL_TEXTURE0);
	glBegin(GL_QUADS);
	double tex_coords[][4]={	{0.0f, 1.0f, 0.0f, 1.0f},
								{1.0f, 1.0f, 0.0f, 1.0f},
								{1.0f, 0.f, 0.0f, 1.0f},
								{0.0f, 0.0f, 0.0f, 1.0f}};

	if (texture.projection_type == projection_t::perspective) {
	// Calculate homogeneous coordinated
	// Based on: http://www.bitlush.com/posts/arbitrary-quadrilaterals-in-opengl-es-2-0

		const double ax = corners[4] - corners[0];
		const double ay = corners[5] - corners[1];

		const double bx = corners[6] - corners[2];
		const double by = corners[7] - corners[3];

		const double cross = ax * by - ay * bx;
		if (std::abs(cross) > 0.0001) {
			const double cx = corners[0] - corners[2];
			const double cy = corners[1] - corners[3];

			const double s = (ax * cy - ay * cx) / cross;
			if (s > 0 && s < 1) {
				const double t = (bx * cy - by * cx) / cross;
				if (t > 0 && t < 1) {
					double q0 = 1 / (1 - t);
					double q1 = 1 / (1 - s);
					double q2 = 1 / t;
					double q3 = 1 / s;
					tex_coords[0][0]*=q0;
					tex_coords[0][1]*=q0;
					tex_coords[0][3]=q0;

					tex_coords[1][0]*=q1;
					tex_coords[1][1]*=q1;
					tex_coords[1][3]=q1;

					tex_coords[2][0]*=q2;
					tex_coords[2][1]*=q2;
					tex_coords[2][3]=q2;

					tex_coords[3][0]*=q3;
					tex_coords[3][1]*=q3;
					tex_coords[3][3]=q3;
				}
			}
		}
	}



	texture.set_tex_coords(tex_coords[0]);
	glVertex2fv(&corners[0]);

	texture.set_tex_coords(tex_coords[1]);
	glVertex2fv(&corners[2]);

	texture.set_tex_coords(tex_coords[2]);
	glVertex2fv(&corners[4]);

	texture.set_tex_coords(tex_coords[3]);
	glVertex2fv(&corners[6]);
	glEnd();
	if (texture.shader) texture.shader->stop();
	glBindTexture(GL_TEXTURE_2D,0);
	glPopAttrib();

}
void GL::enable_smoothing()
{
	glShadeModel(GL_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

void GL::enable_depth()
{
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
}

void GL::save_state()
{
	GLint matrix;
	glGetIntegerv(GL_MATRIX_MODE,&matrix);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(matrix);
}
void GL::restore_state()
{
	GLint matrix;
	glGetIntegerv(GL_MATRIX_MODE,&matrix);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(matrix);
}

void GL::clear()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}


/** \brief Sets quality of yuv422 texture rendering
 *
 * When set to 0, 422 image gets upsampled to 444 and then rendered. Highest quality.
 * When set to 1, the image will be rendered using two separate textures,
 * 		using full scale Y component, but leading to degradation in Y and V components.
 * When set to 2, the image will be rendered using single texture, effectively
 * 		equal to downsampling Y. Fastest method.
 *
 */
bool GL::prepare_texture(index_t tid, unsigned texid, const uint8_t *data, size_t data_size,
		resolution_t resolution, GLenum tex_mode, GLenum data_mode, bool update,
		GLenum data_type)
{
	GLenum err;
	glGetError();
	if (!update) {
		textures[tid].gen_texture(texid);
		if (use_pbo) {
			textures[tid].gen_buffer(texid);
			log[log::info] << "PBO id: " << textures[tid].pbo[texid];
		}
	}
	glBindTexture(GL_TEXTURE_2D, textures[tid].tid[texid]);
	err = glGetError();
	if (err) {
		log[log::error]<< "Error " << err << " while binding texture";
		return false;
	}
	if (data) {
		size_t min_image_size = resolution.width*resolution.height*get_pixel_size(data_mode, data_type);
		if (min_image_size && (data_size < min_image_size)) {
			log[log::error] << "Not enough data to update texture";
			return false;
		}
	}

	if (use_pbo) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, textures[tid].pbo[texid]);
		if (auto e = glGetError()) log[log::error]<< __LINE__ << "Error " << e;
		if (!update) {
			glBufferData(GL_PIXEL_UNPACK_BUFFER, data_size, nullptr, GL_STREAM_DRAW);
			if (!update) {
				textures[tid].pbo_valid[texid]=false;
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			}
		}

	}
	if (!update) {
		glTexImage2D(GL_TEXTURE_2D, 0, tex_mode, resolution.width, resolution.height, 0, data_mode, data_type, nullptr);
	} else {
		if (!use_pbo || textures[tid].pbo_valid[texid])
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, resolution.width, resolution.height, data_mode, data_type, use_pbo?nullptr:data);
	}
	err = glGetError();
	if (err) {
		log[log::error] << "Error " << err /*<< ":" << glGetString(err) */<< " uploading tex. data";
		return false;
	}

	if (!update) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,32.0);
		err = glGetError();
		if (err) {
			log[log::error] << "Error " << err << " setting texture params";
			return false;
		}
	}
	if (use_pbo) {
//		log[log::info] << "Binding buffer " << textures[tid].pbo[texid];
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, textures[tid].pbo[texid]);
		if (auto e = glGetError()) {
			log[log::error]<< __LINE__ << "Error " << e;
			textures[tid].pbo_valid[texid]=false;
			return false;
		}
		glBufferData(GL_PIXEL_UNPACK_BUFFER, data_size, 0, GL_STREAM_DRAW);
		if (auto e = glGetError()) {
			log[log::error]<< __LINE__ << "Error " << e;
			textures[tid].pbo_valid[texid]=false;
			return false;
		}
		auto ptr = reinterpret_cast<uint8_t*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
		if (auto e = glGetError()) {
			log[log::error]<< __LINE__ << "Error " << e;
			textures[tid].pbo_valid[texid]=false;
			return false;
		}
		if(ptr) {
			std::copy(data, data+data_size, ptr);
			glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER);
			if (auto e = glGetError()) {
				log[log::error]<< __LINE__ << "Error " << e;
				textures[tid].pbo_valid[texid]=false;
				return false;
			}
		}
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		if (auto e = glGetError()) {
			log[log::error]<< __LINE__ << "Error " << e;
			textures[tid].pbo_valid[texid]=false;
			return false;
		}
		textures[tid].pbo_valid[texid]=true;

	}
	return true;
}
bool GL::finish_frame()
{
	glFinish();
	return true;
}

namespace {
using namespace core::raw_format;

template<format_t fmt, GLenum gl_format, GLenum data_type>
core::pVideoFrame read_win_generic(geometry_t geometry)
{
	auto frame = core::RawVideoFrame::create_empty(fmt, geometry.get_resolution());
	glReadPixels(geometry.x, geometry.y, geometry.width, geometry.height,
				gl_format, data_type, PLANE_RAW_DATA(frame, 0));
	return frame;
}


template<format_t fmt>
core::pVideoFrame read_win(geometry_t /* geometry */)
{
	return {};
}

template<>
core::pVideoFrame read_win<rgb24>(geometry_t geometry)
{
	return read_win_generic<rgb24, GL_RGB, GL_UNSIGNED_BYTE>(geometry);
}

template<>
core::pVideoFrame read_win<rgba32>(geometry_t geometry)
{
	return read_win_generic<rgba32, GL_RGBA, GL_UNSIGNED_BYTE>(geometry);
}

template<>
core::pVideoFrame read_win<bgr24>(geometry_t geometry)
{
	return read_win_generic<bgr24, GL_BGR, GL_UNSIGNED_BYTE>(geometry);
}

template<>
core::pVideoFrame read_win<bgra32>(geometry_t geometry)
{
	return read_win_generic<bgra32, GL_BGRA, GL_UNSIGNED_BYTE>(geometry);
}
template<>
core::pVideoFrame read_win<depth8>(geometry_t geometry)
{
	return read_win_generic<depth8, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE>(geometry);
}
template<>
core::pVideoFrame read_win<depth16>(geometry_t geometry)
{
	return read_win_generic<depth16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT>(geometry);
}
template<>
core::pVideoFrame read_win<r8>(geometry_t geometry)
{
	return read_win_generic<r8, GL_RED, GL_UNSIGNED_BYTE>(geometry);
}
template<>
core::pVideoFrame read_win<g8>(geometry_t geometry)
{
	return read_win_generic<g8, GL_GREEN, GL_UNSIGNED_BYTE>(geometry);
}
template<>
core::pVideoFrame read_win<b8>(geometry_t geometry)
{
	return read_win_generic<b8, GL_BLUE, GL_UNSIGNED_BYTE>(geometry);
}
template<>
core::pVideoFrame read_win<y8>(geometry_t geometry)
{
	return read_win_generic<y8, GL_LUMINANCE, GL_UNSIGNED_BYTE>(geometry);
}
template<>
core::pVideoFrame read_win<alpha8>(geometry_t geometry)
{
	return read_win_generic<alpha8, GL_ALPHA, GL_UNSIGNED_BYTE>(geometry);
}

template<>
core::pVideoFrame read_win<r16>(geometry_t geometry)
{
	return read_win_generic<r16, GL_RED, GL_UNSIGNED_SHORT>(geometry);
}
template<>
core::pVideoFrame read_win<g16>(geometry_t geometry)
{
	return read_win_generic<g16, GL_GREEN, GL_UNSIGNED_SHORT>(geometry);
}
template<>
core::pVideoFrame read_win<b16>(geometry_t geometry)
{
	return read_win_generic<b16, GL_BLUE, GL_UNSIGNED_SHORT>(geometry);
}
template<>
core::pVideoFrame read_win<y16>(geometry_t geometry)
{
	return read_win_generic<y16, GL_LUMINANCE, GL_UNSIGNED_SHORT>(geometry);
}
template<>
core::pVideoFrame read_win<alpha16>(geometry_t geometry)
{
	return read_win_generic<alpha16, GL_ALPHA, GL_UNSIGNED_SHORT>(geometry);
}

template<>
core::pVideoFrame read_win<rgb8>(geometry_t geometry)
{
	return read_win_generic<rgb8, GL_RGB, GL_UNSIGNED_BYTE_3_3_2>(geometry);
}
template<>
core::pVideoFrame read_win<rgb16>(geometry_t geometry)
{
	return read_win_generic<rgb16, GL_RGB, GL_UNSIGNED_SHORT_5_6_5>(geometry);
}

}

core::pVideoFrame GL::read_window(geometry_t geometry, format_t format)
{
	using namespace core::raw_format;
	switch (format) {
		case rgb24:
			return read_win<rgb24>(geometry);
		case rgba32:
			return read_win<rgba32>(geometry);
		case bgr24:
			return read_win<bgr24>(geometry);
		case bgra32:
			return read_win<bgra32>(geometry);


		case depth8:
			return read_win<depth8>(geometry);
		case depth16:
			return read_win<depth16>(geometry);

		case r8:
			return read_win<r8>(geometry);
		case g8:
			return read_win<g8>(geometry);
		case b8:
			return read_win<b8>(geometry);
		case alpha8:
			return read_win<alpha8>(geometry);
		case y8:
			return read_win<y8>(geometry);
		case r16:
			return read_win<r16>(geometry);
		case g16:
			return read_win<g16>(geometry);
		case b16:
			return read_win<b16>(geometry);
		case alpha16:
			return read_win<alpha16>(geometry);
		case y16:
			return read_win<y16>(geometry);

		case rgb8:
			return read_win<rgb8>(geometry);
		case rgb16:
			return read_win<rgb16>(geometry);
		default:
			break;
	};
	return {};
}

void GL::set_texture_delta(index_t tid, float dx, float dy)
{
	auto it = textures.find(tid);
	if (it != textures.end()) {
		it->second.dx = clip_value(dx, -1.0, 1.0);
		it->second.dy = clip_value(dy, -1.0, 1.0);
	}
}

}

}

