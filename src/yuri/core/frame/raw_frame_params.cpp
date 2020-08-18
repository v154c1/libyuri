/*!
 * @file 		raw_frame_params.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		15.9.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "raw_frame_types.h"
#include "raw_frame_params.h"
#include "yuri/core/utils.h"
#include <map>
namespace yuri {
namespace core {
namespace raw_format {
namespace {

	mutex	format_info_map_mutex;
	format_t last_user_format = user_start;
	format_info_map_t format_info_map = {
			{r8, 	{r8, 	"Red 8 bit", 		{"R", "R8"},"", 		{{"R",	{8,1}, {8}}} }},
			{r16, 	{r16, 	"Red 16 bit", 		{"R16"}, 	"", 		{{"R", 	{16,1}, {16}}} }},
			{g8, 	{g8, 	"Green 8 bit", 		{"G", "G8"},"", 		{{"G", 	{8,1}, {8}}} }},
			{g16, 	{g16, 	"Green 16 bit",		{"G16"}, 	"", 		{{"G", 	{16,1}, {16}}} }},
			{b8, 	{b8, 	"Blue 8 bit", 		{"B", "B8"},"", 		{{"B", 	{8,1}, {8}}} }},
			{b16, 	{b16, 	"Blue 16 bit", 		{"B16"}, 	"", 		{{"B", 	{16,1}, {16}}} }},
			{y8, 	{y8, 	"Y 8 bit", 			{"Y", "Y8"},"", 		{{"Y", 	{8,1}, {8}}} }},
			{y16, 	{y16, 	"Y 16 bit", 		{"Y16"}, 	"", 		{{"Y", 	{16,1}, {16}}} }},
			{u8, 	{u8, 	"U 8 bit", 			{"U", "U8"},"", 		{{"U", 	{8,1}, {8}}} }},
			{u16, 	{u16, 	"U 16 bit", 		{"U16"}, 	"", 		{{"U", 	{16,1}, {16}}} }},
			{v8, 	{v8, 	"V 8 bit", 			{"V", "V8"},"", 		{{"V", 	{8,1}, {8}}} }},
			{v16, 	{v16, 	"V 16 bit", 		{"V16"}, 	"", 		{{"V", 	{16,1}, {16}}} }},
			{depth8,{depth8,"Depth 8 bit", 		{"Depth", "Depth8"},"", {{"D", 	{8,1}, {8}}} }},
			{depth16,{depth16,"Depth 16 bit", 	{"Depth16"}, 	"", 	{{"D", 	{16,1}, {16}}} }},
			{alpha8,{alpha8,"Alpha 8 bit", 		{"Alpha", "Alpha8"},"", {{"A", 	{8,1}, {8}}} }},
			{alpha16,{alpha16,"Alpha 16 bit", 	{"Alpha16"}, 	"", 	{{"A", 	{16,1}, {16}}} }},
			{rgb8, 	{rgb8, 	"RGB 8 bit (332)", 	{"RGB8", "RGB1"}, "", 	{{"RGB",{8, 1}, {3,3,2}}} }},
			{rgb15,	{rgb15, "RGB 15 bit (555)",	{"RGB15", "RGB555"}, "",{{"RGB",{16, 1}, {5,5,5}}} }},
			{rgb16,	{rgb16, "RGB 16 bit (565)",	{"RGB16", "RGB565"}, "",{{"RGB",{16, 1}, {5,6,5}}} }},
			{rgb24,	{rgb24, "RGB 24 bit",		{"RGB", "RGB24"}, "",	{{"RGB",{24, 1}, {8,8,8}}} }},
			{rgb48,	{rgb48, "RGB 48 bit",		{"RGB48"}, "",			{{"RGB",{48, 1}, {16,16,16}}} }},
			{bgr8, 	{bgr8, 	"BGR 8 bit (233)", 	{"BGR8", "BGR1"}, "", 	{{"BGR",{8, 1}, {2,3,3}}} }},
			{bgr15,	{bgr15, "BGR 15 bit (555)",	{"BGR15", "BGR555"}, "",{{"BGR",{16, 1}, {5,5,5}}} }},
			{bgr16,	{bgr16, "BGR 16 bit (565)",	{"BGR16", "BGR565"}, "",{{"BGR",{16, 1}, {5,6,5}}} }},
			{bgr24,	{bgr24, "BGR 24 bit",		{"BGR", "BGR24"}, "",	{{"BGR",{24, 1}, {8,8,8}}} }},
			{gbr24,	{gbr24, "GBR 24 bit",		{"GBR", "GBR24"}, "",	{{"GBR",{24, 1}, {8,8,8}}} }},
			{bgr48,	{bgr48, "BGR 48 bit",		{"BGR48"}, "",			{{"BGR",{48, 1}, {16,16,16}}} }},

			{rgba16,{rgba16, "RGBA 16 bit (5551)",{"RGBA16"}, "",		{{"RGBA",{16, 1}, {5,5,5,1}}} }},
			{rgba32,{rgba32, "RGBA 32 bit",		{"RGBA32","RGBA"}, "",			{{"RGBA",{32, 1}, {8,8,8,8}}} }},
			{rgba64,{rgba64, "RGBA 64 bit",		{"RGBA64"}, "",			{{"RGBA",{64, 1}, {16,16,16,16}}} }},
			{abgr16,{abgr16, "ABGR 16 bit (5551)",{"ABGR16"}, "",		{{"ABGR",{16, 1}, {5,5,5,1}}} }},
			{abgr32,{abgr32, "ABGR 32 bit",		{"ABGR32"}, "",			{{"ABGR",{32, 1}, {8,8,8,8}}} }},
			{abgr64,{abgr64, "ABGR 64 bit",		{"ABGR64"}, "",			{{"ABGR",{64, 1}, {16,16,16,16}}} }},
			{argb32,{argb32, "ARGB 32 bit",		{"ARGB32"}, "",			{{"ARGB",{32, 1}, {8,8,8,8}}} }},
			{argb64,{argb64, "ARGB 64 bit",		{"ARGB64"}, "",			{{"ARGB",{64, 1}, {16,16,16,16}}} }},
			{bgra32,{bgra32, "BGRA 32 bit",		{"BGRA32"}, "",			{{"BGRA",{32, 1}, {8,8,8,8}}} }},
			{bgra64,{bgra64, "BGRA 64 bit",		{"BGRA64"}, "",			{{"BGRA",{64, 1}, {16,16,16,16}}} }},

			{rgb_r10k,{rgb_r10k,"RGB R10k 32 bit",{"R10K"}, "",			{{"RGB", {32, 1}, {10,10,10}}} }},
			{bgr_r10k,{bgr_r10k,"BGR R10k 32 bit",{"BGR10K"}, "",		{{"BGR", {32, 1}, {10,10,10}}} }},

			{yuv411,{yuv411,"YUV 4:1:1 packed",	{"YUV411"}, "",			{{"YYUYYV",{48, 4}, {8,8,8,8,8,8}}} }},
			{yvu411,{yvu411,"YVU 4:1:1 packed",	{"YVU411"}, "",			{{"YYVYYU",{48, 4}, {8,8,8,8,8,8}}} }},

			{yuyv422,{yuyv422,"YUV 4:2:2 packed (YUYV)",{"YUV","YUYV","YUV422"}, "",{{"YUYV",{32, 2}, {8,8,8,8}}} }},
			{yvyu422,{yvyu422,"YUV 4:2:2 packed (YVYU)",{"YVYU"}, "",	{{"YVYU",{32, 2}, {8,8,8,8}}} }},
			{uyvy422,{uyvy422,"YUV 4:2:2 packed (UYVY)",{"UYVY"}, "",	{{"UYVY",{32, 2}, {8,8,8,8}}} }},
			{vyuy422,{vyuy422,"YUV 4:2:2 packed (VYUY)",{"VYUY"}, "",	{{"VYUY",{32, 2}, {8,8,8,8}}} }},

			{yuv444,{yuv444,"YUV 4:4:4 packed (YUV)",{"YUV444"}, "",	{{"YUV",{24, 1}, {8,8,8}}} }},
			{ayuv4444,{ayuv4444,"YUV 4:4:4:4 packed (AYUV)",{"AYUV4444"}, "",	{{"AYUV",{32, 1}, {8,8,8,8}}} }},
			{yuva4444,{yuva4444,"YUV 4:4:4:4 packed (YUVA)",{"YUVA4444"}, "",	{{"YUVA",{32, 1}, {8,8,8,8}}} }},

			// TODO Well, this is crazy but ...
			{yuv422_v210,{yuv422_v210,"YUV 4:2:2 10bit",{"V210","v210_yuv", "v210_yuyv"}, "",	{{"YUY*VYU*YVY*UYV*",{128, 6}, {10,10,10,2,10,10,10,2,10,10,10,2,10,10,10,2}}} }},
			{yvu422_v210,{yvu422_v210,"YUV 4:2:2 10bit",{"yvu_v210","v210_yvu","v210_yvyu"}, "",	{{"YVY*UYV*YUY*VYU*",{128, 6}, {10,10,10,2,10,10,10,2,10,10,10,2,10,10,10,2}}} }},

			{xyz,{xyz,"XYZ 24bit",{"XYZ", "XYZ24"}, "",					{{"xyz", {24, 1}, {8,8,8}}} }},

			{bayer_rggb,{bayer_rggb,"Bayer pattern RGGB",{"rggb"}, "",	{{"", {8, 1}, {8}}} }},
			{bayer_bggr,{bayer_bggr,"Bayer pattern BGGR",{"bggr"}, "",	{{"", {8, 1}, {8}}} }},
			{bayer_grbg,{bayer_grbg,"Bayer pattern GRBG",{"grbg"}, "",	{{"", {8, 1}, {8}}} }},
			{bayer_gbrg,{bayer_gbrg,"Bayer pattern GBRG",{"gbrg"}, "",	{{"", {8, 1}, {8}}} }},


			{rgb24p,{rgb24p, "RGB 24 bit, planar",{"RGBP", "RGB24P"}, "",{{"R",{8, 1}, {8}},{"G",{8, 1}, {8}},{"B",{8, 1}, {8}}} }},
			{rgb48p,{rgb24p, "RGB 48 bit, planar",{"RGB48P"}, "",		{{"R",{16, 1}, {16}},{"G",{16, 1}, {16}},{"B",{16, 1}, {16}}} }},
			{bgr24p,{bgr24p, "BGR 24 bit, planar",{"BGRP", "BGR24P"}, "",{{"B",{8, 1}, {8}},{"G",{8, 1}, {8}},{"R",{8, 1}, {8}}} }},
			{bgr48p,{bgr24p, "BGR 48 bit, planar",{"BGR48P"}, "",		{{"B",{16, 1}, {16}},{"G",{16, 1}, {16}},{"R",{16, 1}, {16}}} }},
			{gbr24p,{gbr24p, "GBR 24 bit, planar",{"GBRP", "GBR24P"}, "",{{"G",{8, 1}, {8}},{"B",{8, 1}, {8}},{"R",{8, 1}, {8}}} }},
			{gbr48p,{gbr24p, "GBR 48 bit, planar",{"GBR48P"}, "",		{{"G",{16, 1}, {16}},{"B",{16, 1}, {16}},{"R",{16, 1}, {16}}} }},

			{rgba32p,{rgba32p, "RGBA 32 bit, planar",{"RGBAP", "RGBA32P"}, "",{{"R",{8, 1}, {8}},{"G",{8, 1}, {8}},{"B",{8, 1}, {8}},{"A",{8, 1}, {8}}} }},
			{rgba64p,{rgba64p, "RGBA 64 bit, planar",{"RGB64P"}, "",		{{"R",{16, 1}, {16}},{"G",{16, 1}, {16}},{"B",{16, 1}, {16}},{"A",{16, 1}, {16}}} }},
			{abgr32p,{abgr32p, "ABGR 32 bit, planar",{"ABGRP", "ABGR32P"}, "",{{"A",{8, 1}, {8}},{"B",{8, 1}, {8}},{"G",{8, 1}, {8}},{"R",{8, 1}, {8}}} }},
			{abgr64p,{abgr64p, "ABGR 64 bit, planar",{"ABGR64P"}, "",		{{"A",{16, 1}, {16}},{"B",{16, 1}, {16}},{"G",{16, 1}, {16}},{"R",{16, 1}, {16}}} }},

			{yuv444p,{yuv444p, "YUV 4:4:4 24 bit, planar",{"YUV444P"}, "",{{"Y",{8, 1}, {8}, 1, 1},{"U",{8, 1}, {8}, 1, 1},{"V",{8, 1}, {8}}} }},
			{yuv422p,{yuv422p, "YUV 4:2:2 16 bit, planar",{"YUV422P"}, "",{{"Y",{8, 1}, {8}, 1, 1},{"U",{8, 1}, {8}, 2, 1},{"V",{8, 1}, {8}, 2, 1}} }},
			{yuv420p,{yuv420p, "YUV 4:2:0 12 bit, planar",{"YUV420P"}, "",{{"Y",{8, 1}, {8}, 1, 1},{"U",{8, 1}, {8}, 2, 2},{"V",{8, 1}, {8}, 2, 2}} }},
			{yuv411p,{yuv411p, "YUV 4:1:1 9 bit, planar",{"YUV411P"}, "",{{"Y",{8, 1}, {8}, 1, 1},{"U",{8, 1}, {8}, 4, 1},{"V",{8, 1}, {8}, 4, 1}} }},


			// Custom formats
			{mvtp_v210,{mvtp_v210, "MVTP YUV (v210) 10bit",{"MVTP"}, "",{{"",{160, 8}, {10}}} }},
	};



	bool do_add_format(const raw_format_t& info)
	{
		auto result = format_info_map.insert({info.format, info});
		return result.second;
	}

}


bool add_format(const raw_format_t& info)
{
	lock_t _(format_info_map_mutex);
	return do_add_format(info);
}

const raw_format_t& get_format_info(format_t format)
{
	lock_t _(format_info_map_mutex);
	auto it = format_info_map.find(format);
	if (it == format_info_map.end()) throw std::runtime_error("Unknown format");
	return it->second;
}

format_t new_user_format()
{
	lock_t _(format_info_map_mutex);
	return last_user_format++;
}

format_t parse_format(const std::string& name)
{
	lock_t _(format_info_map_mutex);
	for (const auto& fmt: format_info_map) {
		for (const auto& fname: fmt.second.short_names) {
			if (iequals(fname, name)) {
				return fmt.first;
			}
		}
	}
	return unknown;
}



format_info_map_t::const_iterator formats::begin() const
{
	return format_info_map.begin();
}
format_info_map_t::const_iterator formats::end() const
{
	return format_info_map.end();
}
}
}
}




