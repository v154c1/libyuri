/*!
 * @file 		Box.h
 */

#ifndef BOX_H_
#define BOX_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/event/BasicEventConsumer.h"


namespace yuri {
namespace draw {

class Box: public core::SpecializedIOFilter<core::RawVideoFrame>,
public event::BasicEventConsumer
{
	using base_type = core::SpecializedIOFilter<core::RawVideoFrame>;
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	Box(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~Box() noexcept;
private:
	virtual core::pFrame do_special_single_step(core::pRawVideoFrame frame) override;
	virtual bool set_param(const core::Parameter& param) override;
	virtual bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;


	size_t thickness_;
};

} /* namespace draw */
} /* namespace yuri */
#endif /* BOX_H_ */
