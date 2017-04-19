/*!
 * @file 		h264_helper.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		19. 4. 2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2017
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SRC_MODULES_RAWAVFILE_H264_HELPER_H_
#define SRC_MODULES_RAWAVFILE_H264_HELPER_H_

#include "yuri/core/frame/CompressedVideoFrame.h"
#include <vector>
namespace yuri {
namespace h264 {

void fill_frame_with_start(format_t format, uint8_t* dd, const uint8_t* data, size_t size);
core::pCompressedVideoFrame prepare_frame_with_start(format_t format, resolution_t res, const uint8_t* data, size_t size);

size_t h264_extradata_size(const uint8_t* extradata);
bool h264_fill_extradata(const uint8_t* extradata, core::pCompressedVideoFrame& f);

std::vector<core::pCompressedVideoFrame> get_extradata_frames(const uint8_t* extradata, format_t format, resolution_t res);
}
}

#endif /* SRC_MODULES_RAWAVFILE_H264_HELPER_H_ */
