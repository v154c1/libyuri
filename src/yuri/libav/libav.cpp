/*!
 * @file 		libav.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		29.10.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "libav.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/raw_audio_frame_types.h"

extern "C" {
	#include <libavformat/avformat.h>
}


#include <unordered_map>
#include <map>
#include <cassert>
#include <atomic>
namespace yuri {
namespace libav {

namespace {
using namespace yuri::core::compressed_frame;
std::unordered_map<format_t, AVCodecID> yuri_codec_map = {
		{mpeg2, 					AV_CODEC_ID_MPEG2VIDEO},
		{mpeg1, 					AV_CODEC_ID_MPEG1VIDEO},
//		{YURI_VIDEO_HUFFYUV, 		CODEC_ID_HUFFYUV},
		{dv, 						AV_CODEC_ID_DVVIDEO},
		{mpeg2ts,					AV_CODEC_ID_MPEG2TS},
		{mjpg,						AV_CODEC_ID_MJPEG},
		{h264,						AV_CODEC_ID_H264},
		{avc1,						AV_CODEC_ID_H264},
		{h265,						AV_CODEC_ID_H265},
//		{YURI_VIDEO_FLASH,			CODEC_ID_FLASHSV2},
//		{YURI_VIDEO_DIRAC,			CODEC_ID_DIRAC},
//		{YURI_VIDEO_H263,			CODEC_ID_H263},
//		{YURI_VIDEO_H263PLUS,		CODEC_ID_H263P},
		{theora,					AV_CODEC_ID_THEORA},
		{vp8,						AV_CODEC_ID_VP8},
		{hap,						AV_CODEC_ID_HAP},
		{core::raw_audio_format::signed_16bit,
									AV_CODEC_ID_PCM_S16LE},
        {core::raw_audio_format::signed_32bit,
                                    AV_CODEC_ID_PCM_S32LE}
};
}
namespace {
using namespace yuri::core::raw_format;
std::unordered_map<format_t, AVPixelFormat> yuri_pixel_map = {
		{rgb8,			 			AV_PIX_FMT_RGB8},
		{bgr8,			 			AV_PIX_FMT_BGR8},
		{rgb16,						AV_PIX_FMT_RGB555},
		{rgb16,						AV_PIX_FMT_RGB565},
		{bgr15,						AV_PIX_FMT_BGR555},
		{bgr16,						AV_PIX_FMT_BGR565},
		{rgb24,			 			AV_PIX_FMT_RGB24},
		{bgr24,			 			AV_PIX_FMT_BGR24},
		{rgba32,		 			AV_PIX_FMT_RGBA},
		{argb32,		 			AV_PIX_FMT_ARGB},
		{bgra32,		 			AV_PIX_FMT_BGRA},
		{abgr32,		 			AV_PIX_FMT_ABGR},
		{rgb48,						AV_PIX_FMT_RGB48},
		{bgr48,						AV_PIX_FMT_BGR48},
		{gbr24p,					AV_PIX_FMT_GBR24P},
#ifdef AV_PIX_FMT_RGBA64
		{rgba64,					AV_PIX_FMT_RGBA64},
#endif
#ifdef AV_PIX_FMT_BGRA64
		{bgra64,					AV_PIX_FMT_BGRA64},
#endif

		{y8,						AV_PIX_FMT_GRAY8},
		{y16,						AV_PIX_FMT_GRAY16},
		{yuv420p,					AV_PIX_FMT_YUV420P},
		{yuv422p,					AV_PIX_FMT_YUV422P},

		{yuv444p,					AV_PIX_FMT_YUV444P},
		{yuyv422,					AV_PIX_FMT_YUYV422},
		{uyvy422,					AV_PIX_FMT_UYVY422},

		{yuv411p,					AV_PIX_FMT_YUV411P},

		{yuv422_v210,				AV_PIX_FMT_YUV422P10LE},

        {nv12,				        AV_PIX_FMT_NV12},
};

using namespace yuri::core::raw_audio_format;
std::unordered_map<format_t, AVSampleFormat> yuri_audio_format_map = {
        {unsigned_8bit,				AV_SAMPLE_FMT_U8},
		{signed_16bit, 				AV_SAMPLE_FMT_S16},
        {signed_32bit, 				AV_SAMPLE_FMT_S32},
		{float_32bit,				AV_SAMPLE_FMT_FLT},
		{float_64bit,				AV_SAMPLE_FMT_DBL},
        {unsigned_8bit_planar,		AV_SAMPLE_FMT_U8P},
		{signed_16bit_planar,		AV_SAMPLE_FMT_S16P},
        {signed_32bit_planar,		AV_SAMPLE_FMT_S32P},
		{float_32bit_planar,		AV_SAMPLE_FMT_FLTP},
		{float_64bit_planar,		AV_SAMPLE_FMT_DBLP},
};


std::map<AVPixelFormat, AVPixelFormat> yuri_pixel_special_map = {
{AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUV420P},
{AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_YUV422P},
{AV_PIX_FMT_YUVJ444P, AV_PIX_FMT_YUV444P},
};

std::mutex libav_initialization_mutex;
std::atomic<bool> libav_initialized {false};

}

lock_t get_libav_lock()
{
	return lock_t (libav_initialization_mutex);
}

void init_libav()
{
	if (libav_initialized) return;
//	lock_t _(libav_initialization_mutex);
	auto _ = get_libav_lock();
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
    // av_register_all() is deprecated starting with version 58.9.100
	av_register_all();
#endif
	libav_initialized = true;
}


AVPixelFormat avpixelformat_from_yuri(yuri::format_t format)
{
	auto it = yuri_pixel_map.find(format);
	return (it==yuri_pixel_map.end())?AV_PIX_FMT_NONE:it->second;
}
AVCodecID avcodec_from_yuri_format(yuri::format_t codec)
{
	auto it = yuri_codec_map.find(codec);
	return (it==yuri_codec_map.end())?AV_CODEC_ID_NONE:it->second;
}

AVSampleFormat avsampleformat_from_yuri(yuri::format_t format)
{
	auto it = yuri_audio_format_map.find(format);
	return (it==yuri_audio_format_map.end())?AV_SAMPLE_FMT_NONE:it->second;
}

yuri::format_t yuri_pixelformat_from_av(AVPixelFormat format)
{
	auto it = yuri_pixel_special_map.find(format);
	if (it != yuri_pixel_special_map.end()) format = it->second;

	for (auto f: yuri_pixel_map) {
		if (f.second == format) return f.first;
	}
	return 0;
}

yuri::format_t yuri_audio_from_av(AVSampleFormat format)
{
	for (auto f: yuri_audio_format_map) {
		if (f.second == format) return f.first;
	}
	return 0;
}

yuri::format_t yuri_format_from_avcodec(AVCodecID codec)
{
	for (auto f: yuri_codec_map) {
		if (f.second == codec) return f.first;
	}
	return 0;
}

core::pRawVideoFrame yuri_frame_from_av(const AVFrame& av_frame)
{
	format_t fmt = libav::yuri_pixelformat_from_av(static_cast<AVPixelFormat>(av_frame.format));
	if (fmt == 0) return {};

	core::pRawVideoFrame frame = core::RawVideoFrame::create_empty(fmt, {static_cast<dimension_t>(av_frame.width), static_cast<dimension_t>(av_frame.height)}, true);
	const auto& fi = core::raw_format::get_format_info(fmt);
	for (size_t i=0;i<4;++i) {
		if ((av_frame.linesize[i] == 0) || (!av_frame.data[i])) break;
		if (i >= frame->get_planes_count()) {
//			log[log::warning] << "BUG? Inconsistent number of planes";
			break;
		}

		size_t line_size = PLANE_DATA(frame,i).get_line_size();//av_frame.width/fi.planes[i].sub_x;
		size_t lines = av_frame.height/fi.planes[i].sub_y;
//				log[log::info] << "Filling plane " << i << ", line size: " << line_size << ", lines: "<<lines;
		assert(line_size <= static_cast<yuri::size_t>(av_frame.linesize[i]));
		//assert(av_frame->linesize[i]*height <= PLANE_SIZE(frame,i));
		for (size_t l=0;l<lines;++l) {
			std::copy(av_frame.data[i]+l*av_frame.linesize[i],
						av_frame.data[i]+l*av_frame.linesize[i]+line_size,
						PLANE_RAW_DATA(frame,i)+l*line_size);
		}
	}
	return frame;
}

}
}



