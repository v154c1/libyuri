/*!
 * @file 		OnepcProtocolCoordinator.cpp
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		4. 5. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "OnepcProtocolCoordinator.h"
#include "yuri/event/EventHelpers.h"

namespace yuri {
namespace synchronization {

core::Parameters OnepcProtocolCoordinator::configure()
{
    core::Parameters p = core::IOFilter::configure();
    p["frame_index"]["Using default frame index."]=false;
    return p;
}


IOTHREAD_GENERATOR(OnepcProtocolCoordinator)

OnepcProtocolCoordinator::OnepcProtocolCoordinator(log::Log &log_, core::pwThreadBase parent,const core::Parameters &parameters):
    core::IOFilter(log_, parent, std::string("onepc_protocol_coordinator")), event::BasicEventProducer(log),
    gen_(std::random_device()()), dis_(1,999999), id_(dis_(gen_)), frame_no_(1), use_index_frame_(true)
{
    IOTHREAD_INIT(parameters)
}

OnepcProtocolCoordinator::~OnepcProtocolCoordinator() noexcept{}


core::pFrame OnepcProtocolCoordinator::do_simple_single_step(core::pFrame frame)
{
	if(use_index_frame_){
		emit_event("perform", prepare_event(id_, frame->get_index()));
	} else {
		emit_event("perform", prepare_event(id_, frame_no_));
		++frame_no_;
	}
	return std::move(frame);
}

event::pBasicEvent OnepcProtocolCoordinator::prepare_event(const uint64_t& id_sender, const index_t& data){
    std::vector<event::pBasicEvent> vec;
    vec.push_back(std::make_shared<event::EventInt>(id_sender));
    vec.push_back(std::make_shared<event::EventInt>(data));
    return std::make_shared<event::EventVector>(std::move(vec));
}

bool OnepcProtocolCoordinator::set_param(const core::Parameter &parameter)
{
    if(assign_parameters(parameter)
            (use_index_frame_, "frame_index"))
        return true;
    return core::IOFilter::set_param(parameter);
}



}
}
