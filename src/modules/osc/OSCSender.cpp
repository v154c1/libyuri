/*!
 * @file 		OSCSender.cpp
 * @author 		<Your name>
 * @date		21.05.2014
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#include "OSCSender.h"
#include "OSC.h"
#include "yuri/core/Module.h"
#include "yuri/core/socket/DatagramSocketGenerator.h"

namespace yuri {
namespace osc {


IOTHREAD_GENERATOR(OSCSender)


core::Parameters OSCSender::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("OSCSender");
	p["address"]["Remote address"]="127.0.0.1";
	p["socket_type"]="yuri_udp";
	p["port"]=57120;
	return p;
}


OSCSender::OSCSender(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,1,1,std::string("osc_sender")),event::BasicEventConsumer(log)
{
	IOTHREAD_INIT(parameters)
}

OSCSender::~OSCSender() noexcept
{
}

void OSCSender::run()
{
	log[log::info] << "Initializing socket of type '"<< socket_type_ << "'";
	socket_ = core::DatagramSocketGenerator::get_instance().generate(socket_type_,log,"");
	log[log::info] << "Binding socket";
	if (!socket_->connect(address_,port_)) {
		log[log::fatal] << "Failed to bind socket!";
		request_end(core::yuri_exit_interrupted);
		return;
	}
	log[log::info] << "Socket initialized";

	while (still_running()) {
		wait_for_events(get_latency());
		process_events();
	}
}
bool OSCSender::set_param(const core::Parameter& param)
{
	if (param.get_name() == "socket_type") {
		socket_type_ = param.get<std::string>();
	} else if (param.get_name() == "port") {
		port_ = param.get<uint16_t>();
	} else if (param.get_name() == "address") {
		address_ = param.get<std::string>();
	} else {
	    return core::IOThread::set_param(param);
	}
	return true;
}

bool OSCSender::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	std::string message=encode_osc(event_name, event, true);
//	log[log::info] << "message size " << message.size();
	if (!message.empty()) {
		socket_->send_datagram(message);
	}
	return true;
}
} /* namespace osc_sender */
} /* namespace yuri */
