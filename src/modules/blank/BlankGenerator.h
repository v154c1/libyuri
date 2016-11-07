/*!
 * @file 		BlankGenerator.h
 * @author 		Zdenek Travnicek
 * @date 		11.9.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef BLANKGENERATOR_H_
#define BLANKGENERATOR_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/core/utils/color.h"
namespace yuri {

namespace blank {

class BlankGenerator: public core::IOThread, public event::BasicEventConsumer {
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	BlankGenerator(log::Log &log_, core::pwThreadBase parent, const core::Parameters& parameters);
	virtual ~BlankGenerator() noexcept = default;
private:
	void run() override;
	bool set_param(const core::Parameter &p) override;
	core::pRawVideoFrame generate_frame(format_t format, resolution_t resolution, core::color_t color);
	virtual bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;
	timestamp_t next_time_;
	float fps_;
	resolution_t resolution_;
	yuri::format_t format_;
	core::color_t color_;
	core::pRawVideoFrame frame_cache_;
};

}

}

#endif /* BLANKGENERATOR_H_ */
