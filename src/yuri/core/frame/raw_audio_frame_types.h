/*!
 * @file 		raw_audio_frame_types.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		21.10.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef RAW_AUDIO_FRAME_TYPES_H_
#define RAW_AUDIO_FRAME_TYPES_H_

#include "yuri/core/utils/new_types.h"
namespace yuri {
namespace core {

namespace raw_audio_format {

const format_t unknown			= 0;
const format_t unsigned_8bit 	= 0x20000;
const format_t signed_16bit 	= 0x20001;
const format_t unsigned_16bit 	= 0x20002;
const format_t signed_24bit 	= 0x20003;
const format_t unsigned_24bit 	= 0x20004;
const format_t signed_32bit 	= 0x20005;
const format_t unsigned_32bit 	= 0x20006;
const format_t signed_48bit 	= 0x20007;
const format_t unsigned_48bit 	= 0x20008;
const format_t float_32bit 	 	= 0x20009;
const format_t float_64bit 	 	= 0x2000a;

const format_t signed_16bit_be 	= 0x20011;
const format_t unsigned_16bit_be= 0x20012;
const format_t signed_24bit_be 	= 0x20013;
const format_t unsigned_24bit_be= 0x20014;
const format_t signed_32bit_be 	= 0x20015;
const format_t unsigned_32bit_be= 0x20016;
const format_t signed_48bit_be 	= 0x20017;
const format_t unsigned_48bit_be= 0x20018;
const format_t float_32bit_be 	= 0x20019;
const format_t float_64bit_be 	= 0x2000a;

const format_t unsigned_8bit_planar
								= 0x20030;
const format_t signed_16bit_planar
								= 0x20031;

const format_t signed_32bit_planar
								= 0x20035;

const format_t float_32bit_planar
								= 0x20039;
const format_t float_64bit_planar
								= 0x2003a;

}
}
}


#endif /* RAW_AUDIO_FRAME_TYPES_H_ */
