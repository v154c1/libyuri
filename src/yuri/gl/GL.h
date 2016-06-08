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
//typedef yuri::shared_ptr<class WindowBase> pWindowBase;

enum class projection_t {
	none,
	perspective,
//	quadlinerar
};

struct texture_info_t {
	GLuint tid[8];
	GLdouble tx, ty, dx, dy;
	bool flip_x, flip_y;
	bool keep_aspect;
	std::shared_ptr<GLProgram> shader;

	format_t format;
	GLint texture_units[8];
	GLint uniform_tx, uniform_ty, uniform_dx, uniform_dy, uniform_flip_x, uniform_flip_y;
	size_t wh;
	projection_t projection_type;
	GLuint pbo[8];
	bool pbo_valid[8];
	texture_info_t():tx(0.0f),ty(0.0f),dx(0.0), dy(0.0), flip_x(false),
			flip_y(false),keep_aspect(false),format(0),wh(0),
			projection_type(projection_t::perspective) {
		for (int i=0;i<8;++i) {
			tid[i]=static_cast<GLuint>(-1);
			pbo[i]=static_cast<GLuint>(-1);
			pbo_valid[i]=false;
			texture_units[i]=-1;
		}
	}
	void load_texture_units(){
		char n[]="texX\0x00";
		for (int i=0;i<8;++i) {
			n[3]='0'+i;
			std::string name = std::string(n);
			texture_units[i]=shader->get_uniform(name);
		}
		uniform_dx = shader->get_uniform("dx");
		uniform_dy = shader->get_uniform("dy");
		uniform_tx = shader->get_uniform("tx");
		uniform_ty = shader->get_uniform("ty");
		uniform_flip_x = shader->get_uniform("flip_x");
		uniform_flip_y = shader->get_uniform("flip_y");
	}
	void bind_texture_units() {
		for (int i=0;i<8;++i) {
			if (texture_units[i]<0) continue;
			glActiveTexture(GL_TEXTURE0+i);
			glBindTexture(GL_TEXTURE_2D,tid[i]);
			//std::cerr << "setting uniform " << texture_units[i] << " to " << i << endl;
			shader->set_uniform_sampler(texture_units[i],i);
		}
		shader->set_uniform_float(uniform_dx, dx>0?dx*tx:0);
		shader->set_uniform_float(uniform_dy, dy>0?dy*ty:0);
		shader->set_uniform_float(uniform_tx, tx * (1.0 - std::fabs(dx)));
		shader->set_uniform_float(uniform_ty, ty * (1.0 - std::fabs(dy)));
		shader->set_uniform_int(uniform_flip_x, flip_x);
		shader->set_uniform_int(uniform_flip_y, flip_y);
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
	void generate_texture(index_t tid, const core::pFrame& frame, bool flip_x = false, bool flip_y = false);
	void generate_empty_texture(index_t tid, format_t fmt, resolution_t resolution);
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
