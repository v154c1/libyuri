/*!
 * @file 		libav.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		29.10.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef LIBAV_H_
#define LIBAV_H_
#include "yuri/core/utils/new_types.h"
#include "yuri/core/forward.h"
extern "C" {
	#include "libavcodec/avcodec.h"
	#include <libavutil/opt.h>
}

#if LIBAVUTIL_VERSION_MAJOR < 52
#define YURI_USING_LEGACY_FFMPEG 1
#include "legacy_libav.h"
#else
#endif
namespace yuri {
namespace libav {

/*!
 * Initializes libav (registers codecs etc.) Can be safely called from multiple threads and multiple times.
 */
void init_libav();

AVPixelFormat avpixelformat_from_yuri(yuri::format_t format);
AVCodecID avcodec_from_yuri_format(yuri::format_t codec);
AVSampleFormat avsampleformat_from_yuri(yuri::format_t format);

yuri::format_t yuri_pixelformat_from_av(AVPixelFormat format);
yuri::format_t yuri_format_from_avcodec(AVCodecID codec);
yuri::format_t yuri_audio_from_av(AVSampleFormat format);

core::pRawVideoFrame yuri_frame_from_av(const AVFrame& frame);

lock_t get_libav_lock();

template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
get_opt(void* obj, const char* name, int flags = 0)
{
	if (!obj) {
		throw std::runtime_error("no obj");
	}
	int64_t val = 0;
	if (av_opt_get_int(obj, name, flags, &val) < 0) {
		throw std::runtime_error(std::string{"error querying parameter "}+name);
	}
	return static_cast<T>(val);
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type
get_opt(void* obj, const char* name, int flags = 0)
{
	if (!obj) {
		throw std::runtime_error("no obj");
	}
	double val = 0.0;
	if (av_opt_get_double(obj, name, flags, &val) < 0) {
		throw std::runtime_error(std::string{"error querying parameter "}+name);
	}
	return val;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, bool>::type
set_opt(void* obj, const char* name, T&& value, int flags = 0)
{
	if (!obj || av_opt_set_int(obj, name, static_cast<int64_t>(value), flags) < 0) {
		return false;
	}
	return true;
}
template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
set_opt(void* obj, const char* name, T&& value, int flags = 0)
{
	if (!obj || av_opt_set_double(obj, name, static_cast<double>(value), flags) < 0) {
		return false;
	}
	return true;
}

}
}


#endif /* LIBAV_H_ */
