/*!
 * @file 		compressed_frame_params.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		4.10.2013
 * @date		21.11.2013
 * @copyright	CESNET, z.s.p.o, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "compressed_frame_types.h"
#include "compressed_frame_params.h"
#include "yuri/core/utils.h"

namespace yuri {
namespace core {
namespace compressed_frame {

namespace {
	mutex	format_info_map_mutex;
	format_t last_user_format = user_start;
	comp_format_info_map_t format_info_map = {
			{unidentified, 	{unidentified,	"Undentified compressed format", {"NONE","UNKNOWN"}, {} }},
			{jpeg, 	{jpeg,	"JPEG", {"JPG","JPEG"}, {"image/jpeg"} }},
			{mjpg, 	{mjpg,	"Motion JPEG", {"MJPG","MJPEG"}, {"video/mjpeg"} }},
			{png, 	{png,	"PNG", {"PNG"}, {"image/png"} }},
			{h264, 	{h264,	"H.264", {"H264"}, {"video/h264"} }},
			{vp8, 	{vp8,	"VP8", {"VP8"}, {"video/vp8"} }},
			{dxt1,  {dxt1,  "DXT1", {"DXT1"}, {}}},
			{dxt5,  {dxt5,  "DXT5", {"DXT5"}, {}}},
			{dv, 	{dv,	"DV", {"DV"}, {"video/dv"} }},
			{mpeg2,	{mpeg2,	"MPEG 2", {"MPEG2","MPG2","HDV"}, {"video/mpeg2"} }},
			{mpeg2ts,{mpeg2ts,"MPEG 2 Transport Stream", {"MPEG2TS","MPGTS","TS"}, {}}},
			{huffyuv,{huffyuv,"HUFFYUV", {"HUFFYUV","HUFF"}, {}}},
			{mpeg1,	{mpeg1,	"MPEG 1", {"MPEG1","MPG","MPEG"}, {}}},
			{ogg,	{ogg,	"OGG", {"MPEG1"}, {"application/ogg"}}},
			{theora,{theora,"THEORA", {"THEORA"}, {"video/theora"}}},
			{h265,	{h265,	"H.265", {"H265","H.265"}, {"video/h265"}}},
			{avc1, 	{avc1,	"H.264 (AVC1)", {"AVC1"}, {"video/avc1"} }},
            {hap,{hap,"HAP", {"HAP"}, {"video/hap"} }},
            {ycocg_dxt5,{ycocg_dxt5,"YCoCg DXT5", {"YCoCg_DXT"}, {"video/ycocg_dxt5"} }},
			{jpegxs,{jpegxs,"JPEGXS", {"JPEGXS"}, {"video/jpegxs"} }},
	};

	bool do_add_format(const compressed_frame_info_t& info)
	{
		auto result = format_info_map.insert({info.format, info});
		return result.second;
	}

}


bool add_format(const compressed_frame_info_t& info)
{
	lock_t _(format_info_map_mutex);
	return do_add_format(info);
}

const compressed_frame_info_t& get_format_info(format_t format)
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

format_t get_format_from_mime(const std::string& mime)
{
	lock_t _(format_info_map_mutex);
	for (const auto& fmt: format_info_map) {
		for (const auto& fname: fmt.second.mime_types) {
			if (iequals(fname, mime)) {
				return fmt.first;
			}
		}
	}
	return unknown;
}


comp_format_info_map_t::const_iterator formats::begin() const
{
	return format_info_map.begin();
}
comp_format_info_map_t::const_iterator formats::end() const
{
	return format_info_map.end();
}

}
}
}


