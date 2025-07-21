/*!
 * @file 		GLShader.cpp
 * @author 		Zdenek Travnicek
 * @date 		23.1.2012
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2012 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "GLShader.h"
#include <fstream>
#include <iostream>
#include <GL/glext.h>
#include <algorithm>

namespace yuri {

namespace gl {

GLShader::GLShader(log::Log &log_, GLenum type):log(log_),type(type),shader_text(nullptr),
		shader_size(0),shader_object(0)
{
	log.set_label("[GLShader] ");
}

GLShader::~GLShader() noexcept
{
}


bool GLShader::load_file(const std::string& filename)
{
	std::ifstream file;
	file.open(filename.c_str(),std::ios::binary);
	if (file.bad()) return false;
	file.seekg(0,std::ios::end);
	shader_size = file.tellg();
	file.seekg(0,std::ios::beg);
	if (!shader_size) return false;
	if (shader_text) delete [] shader_text;
	shader_text = new GLchar[shader_size];
	file.read(shader_text, shader_size);
	return true;
}
bool GLShader::load(const std::string& text)
{
	shader_size = text.size();
	if (!shader_size) return false;
	if (shader_text) delete [] shader_text;
	shader_text = new GLchar[++shader_size+1];
	std::fill(shader_text, shader_text+shader_size,0);
	std::copy_n(text.c_str(), shader_size-1, shader_text);
	shader_text[shader_size] = 0;
	return true;
}

bool GLShader::compile()
{
	shader_object = glCreateShader(type);
	glShaderSource(shader_object,1,(const GLchar**)&shader_text,&shader_size);
	glCompileShader(shader_object);
	GLint compiled;
    glGetShaderiv(shader_object, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_TRUE) {
		log[log::debug] << "Shader " << shader_object << " compiled without problems.";
		return true;
	}
	GLint blen = 0;
	GLsizei slen = 0;
	glGetShaderiv(shader_object, GL_INFO_LOG_LENGTH , &blen);
	if (blen > 1)
	{
	 GLchar* compiler_log = new GLchar[blen];
    glGetShaderInfoLog(shader_object, blen, &slen, compiler_log);
	 log[log::error] << "compiler_log:" <<  compiler_log;
	 delete [] compiler_log;
	}
	return false;
}

GLuint GLShader::get_shader()
{
	return shader_object;
}


}

}
