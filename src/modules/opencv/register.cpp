/*
 * register.cpp
 *
 *  Created on: 4.11.2013
 *      Author: neneko
 */

#include "OpenCVConvert.h"
#include "OpenCVFaceDetect.h"
#include "OpenCVSource.h"
#include "OpenCVRotate.h"
#include "yuri/core/thread/IOThreadGenerator.h"
#include "yuri/core/thread/ConverterRegister.h"

namespace yuri {
namespace opencv {



MODULE_REGISTRATION_BEGIN("opencv")
		REGISTER_IOTHREAD("opencv_convert",OpenCVConvert)
		for (auto x: convert_format_map) {
			REGISTER_CONVERTER(x.first.first, x.first.second, "opencv_convert", 8);
		}

		REGISTER_IOTHREAD("opencv_face_detection",OpenCVFaceDetect)
		REGISTER_IOTHREAD("opencv_source",OpenCVSource)
		REGISTER_IOTHREAD("opencv_rotate",OpenCVRotate)
MODULE_REGISTRATION_END()

}
}
