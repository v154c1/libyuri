/*!
 * @file 		compressed_frame_types.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		4.10.2013
 * @date		21.11.2013
 * @copyright	CESNET, z.s.p.o, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef COMPRESSED_FRAME_TYPES_H_
#define COMPRESSED_FRAME_TYPES_H_
#include "yuri/core/utils/new_types.h"
namespace yuri {
namespace core {
namespace compressed_frame {

const format_t unknown		= 0;

const format_t unidentified	= 0x10000;
const format_t jpeg			= 0x10001;
const format_t mjpg			= 0x10002;
const format_t png			= 0x10003;
const format_t h264			= 0x10004;
const format_t dxt1			= 0x10005;
const format_t dxt5			= 0x10006;
const format_t vp8			= 0x10007;
const format_t dv			= 0x10008;
const format_t mpeg2		= 0x10009;
const format_t mpeg2ts		= 0x1000a;
const format_t huffyuv		= 0x1000b;
const format_t mpeg1		= 0x1000c;
const format_t ogg			= 0x1000d;
const format_t theora		= 0x1000e;
const format_t h265			= 0x1000f;
// h264 with AVC1 bit stream
const format_t avc1			= 0x10010;

const format_t user_start 	= 0x11000;

}
}
}



#endif /* COMPRESSED_FRAME_TYPES_H_ */
