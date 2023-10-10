/*
 * default_shaders.h
 *
 *  Created on: 31. 10. 2014
 *      Author: neneko
 */

#ifndef DEFAULT_SHADERS_H_
#define DEFAULT_SHADERS_H_

#include <string>

namespace yuri {

namespace gl {

namespace shaders {
namespace {
const std::string vs_default = R"XXX(
void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}
)XXX";

const std::string fs_head = R"XXX(

uniform float tx0;
uniform float ty0;
uniform float tw;
uniform float th;

)XXX";

const std::string fs_main = R"XXX(
void main()
{
	vec2 mapped_coords = transform_coords(gl_TexCoord[0].st / gl_TexCoord[0].w);
	vec2 coord = mapped_coords *vec2(tw, th) + vec2(tx0, ty0);
	vec4 color = get_color(coord);
	gl_FragColor = map_color(color, coord);
}
)XXX";

const std::string fs_default_transform = R"XXX(
vec2 transform_coords(vec2 coord) {
	return coord;
}
)XXX";

const std::string fs_default_map = R"XXX(
vec4 map_color(vec4 color, vec2 coord) {
	return color;
}
)XXX";

std::string get_version_string(int version)
{
	return "#version " + std::to_string(version) + "\n";
}
std::string prepare_vs(int version = 120)
{
	return get_version_string(version) +
		   vs_default;
}
std::string prepare_fs(const std::string& get_color, const std::string& transform, const std::string& color_map, int version = 120)
{
	return	get_version_string(version) +
			fs_head +
			get_color +
			(transform.empty()?fs_default_transform:transform) +
			"\n" +
			(color_map.empty()?fs_default_map:color_map) +
			"\n" +
			fs_main;

}


const std::string fs_get_rgb = R"XXX(
uniform sampler2D tex0;
vec4 get_color(vec2 coord) {
	return texture2D(tex0, coord);
}
)XXX";

/*
mat4 y2rt = mat4(1, 0, 1.371, 0,
	                 1, -.337, -0.698, 0,
	                 1, 1.733,0, 0,
	                 0, 0, 0, 1);
*/

const std::string yuv_to_rgb = R"XXX(
vec4 convert_yuv_rgb(vec4 col) {
	mat4 y2rt = mat4(1, 0, 1.28033, 0,
	                 1, -.21482, -0.38059, 0,
	                 1, 2.12798,0, 0,
	                 0, 0, 0, 1);
	return col*y2rt;
}

)XXX";



const std::string fs_get_yuv444 =
		yuv_to_rgb + R"XXX(
uniform sampler2D tex0;
vec4 get_color(vec2 coord) {
	vec4 col0 = texture2D(tex0, coord);
	vec4 col = vec4(col0.r - 0.0625, col0.g - 0.5, col0.b - 0.5, 1.0);
	return convert_yuv_rgb(col);
}
)XXX";


const std::string fs_get_yuyv422 =
		yuv_to_rgb + R"XXX(
uniform sampler2D tex0, tex1;
vec4 get_color(vec2 coord) {
	vec4 col0 = texture2D(tex0, coord);
	vec4 col1 = texture2D(tex1, coord);
	vec4 yuv = vec4(col1.r - 0.0625, col0.g - 0.5, col0.a - 0.5, 1.0);
	return convert_yuv_rgb(yuv);	
}
)XXX";

const std::string fs_get_yvyu422 =
		yuv_to_rgb + R"XXX(
uniform sampler2D tex0, tex1;
vec4 get_color(vec2 coord) {
	vec4 col0 = texture2D(tex0, coord);
	vec4 col1 = texture2D(tex1, coord);
	vec4 col = vec4(col1.r - 0.0625, col0.a - 0.5, col0.g - 0.5, 1.0);
	return convert_yuv_rgb(col);
}
)XXX";

const std::string fs_get_uyvy422 =
		yuv_to_rgb + R"XXX(
uniform sampler2D tex0, tex1;
vec4 get_color(vec2 coord) {
	vec4 col0 = texture2D(tex0, coord);
	vec4 col1 = texture2D(tex1, coord);
	vec4 col = vec4(col1.a - 0.0625, col0.r - 0.5, col0.b - 0.5, 1.0f);
	return convert_yuv_rgb(col);
}
)XXX";

const std::string fs_get_vyuy422 =
		yuv_to_rgb + R"XXX(
uniform sampler2D tex0, tex1;
vec4 get_color(vec2 coord) {
	vec4 col0 = texture2D(tex0, coord);
	vec4 col1 = texture2D(tex1, coord);
	vec4 col = vec4(col1.a - 0.0625, col0.b - 0.5, col0.r - 0.5, 1.0);
	return convert_yuv_rgb(col);
}
)XXX";

const std::string fs_get_yuv_planar =
		yuv_to_rgb + R"XXX(
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
vec4 get_color(vec2 coord) {
	vec4 col = vec4(
		texture2D(tex0, coord).r,
		texture2D(tex1, coord).r-0.5,
		texture2D(tex2, coord).r-0.5,
		1.0);
	return convert_yuv_rgb(col);
}
)XXX";

    const std::string fs_get_nv12 =
            yuv_to_rgb + R"XXX(
uniform sampler2D tex0;
uniform sampler2D tex1;
vec4 get_color(vec2 coord) {
vec4 col = vec4(
        texture2D(tex0, coord).r,
        texture2D(tex1, coord).r-0.5,
        texture2D(tex1, coord).a-0.5,
        1.0);
return convert_yuv_rgb(col);
}
)XXX";

    const std::string fs_get_ycocg_scaled = R"XXX(
uniform sampler2D tex0;
vec4 get_color(vec2 coord) {
    vec4 color = texture2D(tex0, coord);
    float Co = color.x - ( 0.5 * 256.0 / 255.0 );
    float Cg = color.y - ( 0.5 * 256.0 / 255.0 );
    float Y = color.w;
    return vec4 (
        Y + Co - Cg,
        Y + Cg,
        Y - Co - Cg,
        1.0);
}
)XXX";


}
}
}
}



#endif /* DEFAULT_SHADERS_H_ */
