/*!
 * @file 		avcommon.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		2.7.2017
 * @copyright	Institute of Intermedia, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "avcommon.h"

namespace yuri {
namespace libav {

int libav_thread_type(thread_type_t type) 
{
    switch(type) {
        case thread_type_t::any: return FF_THREAD_SLICE | FF_THREAD_FRAME;
        case thread_type_t::frame: return FF_THREAD_FRAME;
        case thread_type_t::slice: return FF_THREAD_SLICE;
    }
    return 0;
}

thread_type_t parse_thread_type(const std::string& type_string) 
{
    if (type_string == "frame") {
        return thread_type_t::frame;
    } else if (type_string == "slice") {
        return thread_type_t::slice;
    }
    return thread_type_t::any;
    
}
}
}
