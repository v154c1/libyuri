/*!
 * @file 		GL.h
 * @author 		Zdenek Travnicek
 * @date 		31.5.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef GL_H_
#define GL_H_
#include <map>
#include <vector>
#include <array>
#define GL_GLEXT_PROTOTYPES
#include "GLProgram.h"
#include <GL/gl.h>

#include <GL/glext.h>
#include "yuri/core/forward.h"
#include "yuri/core/frame/VideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include <cmath>

namespace yuri {

namespace gl {

enum class projection_t {
	none,
	perspective,
//	quadlinerar
};


/*!
 * Texture info.
 *
 */

struct texture_info_t {
	/// yuri format used for the texture
	format_t format;

	/// texture resolution (allocated texture on GPU)
	resolution_t tex_res;

	/// type of projection (unused?)
	projection_t projection_type;

	/// up to 8 allocated OpenGL textures. Unallocated slots are set to (GLuint)(-1)
	GLuint tid[8];

	/// flip the texture during rendering along X or Y axis.
	bool flip_x, flip_y;

	/// Keep aspect ration when rendering (unused?)
	bool keep_aspect;

	/// Offset of the image in either axis. (i.e. set to dx=0.1, to display skip first 10% of the image)
	GLdouble dx, dy;

	/// 2 points defining a rectangle in the image to be rendered. should be in <0.0, 1.0>
	GLdouble tx0, tx1, ty0, ty1;

	/// Instance of shader used to render this texture
	std::shared_ptr<GLProgram> shader;

	/// Uniform addresses for texture units
	GLint texture_units[8];

	/// Uniform addresses of other parameters
	GLint uniform_tx0,/* uniform_tx1, */uniform_ty0, /*uniform_ty1, */ uniform_tw, uniform_th;//, /* uniform_dx, uniform_dy, */ uniform_flip_x, uniform_flip_y;

	/// Allocated PBOs for asynchronous operations
	GLuint pbo[8];

	/// FLags for each PBO to determine whether is valid
	bool pbo_valid[8];

	texture_info_t():
		format{0},
		tex_res{0, 0},
		projection_type{projection_t::perspective},
		flip_x{false}, flip_y{false},keep_aspect{false},
		dx{0.0}, dy{0.0},
		tx0{0.0}, tx1{1.0}, ty0{0.0}, ty1{1.0},
		uniform_tx0{-1},
		uniform_ty0{-1},
		uniform_tw{-1},uniform_th{-1}
			 {
		for (int i=0;i<8;++i) {
			tid[i]=static_cast<GLuint>(-1);
			pbo[i]=static_cast<GLuint>(-1);
			pbo_valid[i]=false;
			texture_units[i]=-1;
		}
	}

	 /*!
	  * Loads addresses of texture samplers ans uniforms
	  */
	void load_texture_units(){
		char n[]="texX\0x00";
		for (int i=0;i<8;++i) {
			n[3]='0'+i;
			std::string name = std::string(n);
			texture_units[i]=shader->get_uniform(name);
		}
		uniform_tx0 = shader->get_uniform("tx0");
		uniform_ty0 = shader->get_uniform("ty0");
		uniform_tw  = shader->get_uniform("tw");
		uniform_th  = shader->get_uniform("th");

	}
	void bind_texture_units() {
		for (int i=0;i<8;++i) {
			if (texture_units[i]<0) continue;
			glActiveTexture(GL_TEXTURE0+i);
			glBindTexture(GL_TEXTURE_2D,tid[i]);
			shader->set_uniform_sampler(texture_units[i],i);
		}

		const auto width_full = tx1 - tx0;
		const auto height_full  = ty1 - ty0;
		const auto width_offset = width_full * std::abs(dx);
		const auto height_offset = height_full * std::abs(dy);
		const auto width = width_full - width_offset;
		const auto height = height_full - height_offset;

		const auto left = dx > 0.0?tx0+width_offset:tx0;
		const auto right = left + width;
		const auto bottom = dy > 0.0?ty0+height_offset:ty0;
		const auto top = bottom + height;

		shader->set_uniform_float(uniform_tx0, flip_x?right:left);
		shader->set_uniform_float(uniform_ty0, flip_y?top:bottom);
		shader->set_uniform_float(uniform_tw, flip_x?-width:width);
		shader->set_uniform_float(uniform_th, flip_y?-height:height);
	}

	inline void gen_texture(int id) {
		if (tid[id]==static_cast<GLuint>(-1)) glGenTextures(1, tid+id);
	}

	inline void gen_buffer(int id) {
		if (pbo[id]==static_cast<GLuint>(-1)) glGenBuffers(1, pbo+id);
	}
	void set_tex_coords(double  *v) {
		glTexCoord4dv(v);
		for (int i=0;i<8;++i) {
			if (tid[i]!=static_cast<GLuint>(-1)) {
				glMultiTexCoord4dv(GL_TEXTURE0+i,v);
			}
		}
	}

	bool shader_update_needed(const format_t fmt) const {
		return !shader || fmt != format;
	}

	void finish_update(log::Log &log,yuri::format_t fmt,const std::string& vs, const std::string& fs)
	{
		if (format != fmt) {
			format = fmt;
			shader.reset();
		}
		if (!shader) {
			log[log::debug] << "Loading fs:\n"<<fs << "\nAnd vs:\n"<<vs;
			shader = std::make_shared<GLProgram>(log);
			shader->load_shader(GL_VERTEX_SHADER,vs);
			shader->load_shader(GL_FRAGMENT_SHADER,fs);
			shader->link();
			load_texture_units();
		}
	}
};

class GL {
public:
	GL(log::Log &log_);
	virtual ~GL() noexcept;
	std::map<uint,texture_info_t> textures;
	void generate_texture(index_t tid, const core::pFrame& gframe, bool flip_x = false, bool flip_y = false);
	void generate_texture(index_t tid, const format_t frame_format, const resolution_t res, const core::pFrame& gframe, bool flip_x = false, bool flip_y = false, bool force_pow2 = true);
	void generate_empty_texture(index_t tid, format_t fmt, resolution_t resolution);
	void use_remote_texture(index_t tid, GLuint name, const format_t frame_format, const resolution_t res, bool flip_x = false, bool flip_y = false, float tx0 = 0.0f, float ty0 = 0.0f, float tx1 = 1.0f, float ty1 = 1.0f);
	void setup_ortho(GLdouble left=-1.0, GLdouble right=1.0f,
			GLdouble bottom=-1.0, GLdouble top=1.0,
			GLdouble near=-100.0, GLdouble far=100.0);
	void draw_texture(index_t tid);
	static void enable_smoothing();
	static void save_state();
	static void restore_state();
	static void clear();
	void enable_depth();
	bool prepare_texture(index_t tid, unsigned texid, const uint8_t *data, size_t data_size,
			resolution_t resolution, GLenum tex_mode, GLenum data_mode, bool update,
			GLenum data_type = GL_UNSIGNED_BYTE);
	bool finish_frame();
	static core::pVideoFrame read_window(geometry_t geometry, format_t format = core::raw_format::rgb24);


	void set_texture_delta(index_t tid, float dx, float dy);
	log::Log log;

	std::string transform_shader;
	std::string color_map_shader;
	int shader_version_;
	std::array<float,8> corners;
	static mutex big_gpu_lock;
	static std::vector<format_t> get_supported_formats();
	bool use_pbo;
};

}

}

#endif /* GL_H_ */
