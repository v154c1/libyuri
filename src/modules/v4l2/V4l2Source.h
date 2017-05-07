/*!
 * @file 		V4l2Source.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		17.5.2009
 * @date		25.1.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2009 - 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef V4L2SOURCE_H_
#define V4L2SOURCE_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/core/thread/InputThread.h"
#include "v4l2_controls.h"
#include "v4l2_common.h"
#include <iostream>
#if YURI_BSD
#include <sys/videoio.h>
#else
#include <linux/videodev2.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>


namespace yuri {

namespace v4l2 {

struct v4l2_device;


class V4l2Source: public core::IOThread, public event::BasicEventConsumer {
public:

	V4l2Source(log::Log &log_,core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~V4l2Source() noexcept;
	virtual void run() override;
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	static std::vector<core::InputDeviceInfo> enumerate();
private:
	virtual bool set_param(const core::Parameter &param) override;

	std::unique_ptr<v4l2_device> open_device();
	bool prepare_frame(uint8_t *data, yuri::size_t size);
	bool enum_controls();
	virtual bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;


	std::string filename_;
	std::unique_ptr<v4l2_device> device_;
	capture_method_t method_;
	std::vector<buffer_t> buffers_;
	int input_;
	format_t format_;
	resolution_t resolution_;
	fraction_t fps_;

	dimension_t imagesize_;

	bool allow_empty_;
	size_t buffer_free_;
	core::pFrame output_frame_;
	bool combine_frames_;

	std::vector<controls::control_info> controls_;
	std::map<std::string, event::pBasicEvent> control_tmp_;
	bool illuminator_;

	bool repeat_headers_;
	// Used to store SPS/PPS for H264
	std::vector<uint8_t> headers_;
};

}

}
#endif /* V4L2SOURCE_H_ */
