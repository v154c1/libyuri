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

}
}


#endif /* LIBAV_H_ */
