/*!
 * @file 		avcommon.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		2.7.2017
 * @copyright	Institute of Intermedia, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef MODULES_RAWAVFILE_AVCOMMON_H_
#define MODULES_RAWAVFILE_AVCOMMON_H_

#include "yuri/libav/libav.h"

namespace yuri {
namespace libav {
enum class thread_type_t { any, slice, frame };

int libav_thread_type(thread_type_t type) ;
thread_type_t parse_thread_type(const std::string& type_string);
}
    struct AVPacketDeleter {
        void operator()(AVPacket*p) {
            av_packet_unref(p);
            av_packet_free(&p);
        }
    };
}

#endif /* MODULES_RAWAVFILE_AVCOMMON_H_ */