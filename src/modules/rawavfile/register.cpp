/*!
 * @file 		register.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		4.3.2017
 * @copyright	Institute of Intermedia, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "AVDecoder.h"
#include "RawAVFile.h"
#include "yuri/core/Module.h"
#include "RawAVFilePlaylist.h"

namespace yuri {

MODULE_REGISTRATION_BEGIN("avmodules")
    REGISTER_IOTHREAD("rawavsource", rawavfile::RawAVFile)
    REGISTER_IOTHREAD("avdecoder", avdecoder::AVDecoder)
    REGISTER_IOTHREAD("rawavplaylist", rawavfile::RawAVFilePlaylist)
MODULE_REGISTRATION_END()
}
