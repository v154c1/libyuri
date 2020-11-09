/*!
 * @file 		JsonConfig.h
 * @author 		<Your name>
 * @date 		22.01.2015
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#ifndef JSONCONFIG_H_
#define JSONCONFIG_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventProducer.h"
#include "yuri/event/BasicEventConsumer.h"
#include "jsoncpp/json/json.h"
namespace yuri {
namespace json_config {

using event_map = std::map<std::string, event::pBasicEvent>;
class JsonConfig: public core::IOThread, public event::BasicEventConsumer, public event::BasicEventProducer
{
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	JsonConfig(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~JsonConfig() noexcept;
private:
	
	virtual void run() override;
	virtual bool set_param(const core::Parameter& param) override;
	virtual bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;

	std::string filename_;
	event_map event_map_;
	bool readonly_;
	bool autosave_;
};

} /* namespace json_config */
} /* namespace yuri */
#endif /* JSONCONFIG_H_ */
