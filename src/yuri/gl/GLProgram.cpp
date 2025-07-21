/*!
 * @file 		GLProgram.cpp
 * @author 		Zdenek Travnicek
 * @date 		23.1.2012
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2012 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "GLProgram.h"

namespace yuri {

namespace gl {

GLProgram::GLProgram(log::Log &log_):log(log_)
{
	log.set_label("[GLProgram] ");
	program = glCreateProgram();
}


GLProgram::~GLProgram()
{

}

bool GLProgram::attach_shader(GLShader &shader)
{
    if (!glIsShader(shader.get_shader())) {
        log[log::error] << "Invalid shader " << shader.get_shader();
    }
    if (!glIsProgram(program)) {
        log[log::error] << "Invalid programs " << program;
    }
	glAttachShader(program,shader.get_shader());
	GLenum err = glGetError();
	if (err) {
		log[log::error] << "Error " << err << " while attaching shader";
	}
	return true;
}

bool GLProgram::link()
{
	log[log::debug] << "Linking program";
	glLinkProgram(program);
	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked) {
		/*view_matrix = glGetUniformLocation(program,"view_matrix");
		model_matrix = glGetUniformLocation(program,"model_matrix");
		projection_matrix  = glGetUniformLocation(program,"projection_matrix");
		cerr << "vm: " << view_matrix << ", mm: " << model_matrix << ", pm: " << projection_matrix;*/
		log[log::debug] << "Shader program linked";
		return true;
	}
	log[log::warning] << "Shader program NOT linked";
	GLint blen = 0;
	GLsizei slen = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH , &blen);
	if (blen > 1)
	{
		GLchar* compiler_log = new GLchar[blen];
        glGetProgramInfoLog(program, blen, &slen, compiler_log);
		log[log::error] << "compiler_log:" <<  compiler_log;
		delete [] compiler_log;
	}
	return false;
}

void GLProgram::use()
{
	glUseProgram(program);
}
void GLProgram::stop()
{
	glUseProgram(0);
}

bool GLProgram::load_shader_file(GLuint type, const std::string& source)
{
	GLShader shader(log,type);
	log[log::debug]<<"Loading shader";
	if (!shader.load_file(source)) {
	log[log::error] << "Failed to load shader " << source;
		return false;
	}
	log[log::verbose_debug]<<"Compiling " << source;
	if (!shader.compile()) {
	    log[log::error] << "Failed to compile shader "<< source;
	    return false;
	}
	return attach_shader(shader);
}

bool GLProgram::load_shader(GLuint type, const std::string& source)
{
	GLShader shader(log,type);
	log[log::debug]<<"Loading shader";
	log[log::verbose_debug] << source;
	if (!shader.load(source)) {
	log[log::error] << "Failed to load shader " << source;
		return false;
	}
	log[log::debug]<<"Compiling shader";
	log[log::verbose_debug] << source;
	if (!shader.compile()) {
	    log[log::error] << "Failed to compile shader "<< source;
	    return false;
	}
	return attach_shader(shader);
}

void GLProgram::bind_attrib(GLuint index, const std::string& name)
{
	glBindAttribLocation(program,index,name.c_str());
}
GLint GLProgram::get_uniform(const std::string& name)
{
	return glGetUniformLocation(program,const_cast<const char *>(name.c_str()));
}
void GLProgram::set_uniform_sampler(GLint id, GLint value)
{
	return glUniform1i(id,value);
}
void GLProgram::set_uniform_float(GLint id, GLfloat value)
{
	return glUniform1f(id,value);
}
void GLProgram::set_uniform_int(GLint id, GLint value)
{
	return glUniform1i(id,value);
}

}

}
