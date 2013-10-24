/*!
 * @file 		OSCReceiver.cpp
 * @author 		<Your name>
 * @date		13.07.2013
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#include "OSCReceiver.h"
#include "yuri/core/Module.h"
#include "yuri/core/socket/DatagramSocketGenerator.h"

namespace yuri {
namespace osc_receiver {

IOTHREAD_GENERATOR(OSCReceiver)

MODULE_REGISTRATION_BEGIN("osc_receiver")
		REGISTER_IOTHREAD("osc_receiver",OSCReceiver)
MODULE_REGISTRATION_END()

core::Parameters OSCReceiver::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("OSCReceiver");
	p["socket_type"]="uv_udp";
	//p->set_max_pipes(0,0);
	return p;
}


OSCReceiver::OSCReceiver(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,0,0,std::string("osc_receiver")),
event::BasicEventProducer(log),port_(2000),socket_type_("uv_udp")
{
	IOTHREAD_INIT(parameters)


}

OSCReceiver::~OSCReceiver() noexcept
{
}

namespace {

enum class data_type_t {
	unknown_type,
	int32_type,
	float32_type,
	string_type,

};

std::map<char,data_type_t>
data_types = {{'i', data_type_t::int32_type}};

template<class Iterator>
std::string read_string(Iterator& first, const Iterator& last)
{
	std::string str;
	while (first!=last) {
		if (*first == 0) {
			++first;
			break;
		}
		str+=*first;
		++first;
	}
	if (size_t m = (str.size()+1)%4) {
		first=std::min(first+(4-m),last);
	}
//	if (str.empty()) return str;
//	while (first!=last) {
//		if (*first!=0) break;
//		++first;
//	}
	return str;
}
template<class Iterator>
void read_timestamp(Iterator& first, const Iterator& last)
{
	if (std::distance(first,last)<8) {
		first = last;
	} else {
		first+=8;
	}
}
template<class Iterator>
int32_t read_size(Iterator& first, const Iterator& last)
{
	if (std::distance(first,last)<4) {
		first = last;
	} else {
		int32_t size = 0;
		for (int i=0;i<4;++i) {
			size=(size<<8)+(static_cast<int32_t>(*first)&0xFF);
			++first;
		}
		return size;
	}
	return -1;
}

template<class Iterator>
data_type_t read_type(Iterator& first, const Iterator& last)
{
	if (first==last) return data_type_t::unknown_type;
	auto iter = data_types.find(*first);
	++first;
	if (iter == data_types.end()) return data_type_t::unknown_type;
	return iter->second;
}
template<class Iterator>
float read_float(Iterator& first, const Iterator& last)
{
	float f;
	uint8_t *fptr=reinterpret_cast<uint8_t*>(&f);
	ssize_t idx = 3;
	while (first != last && idx>=0) {
		fptr[idx--]=*first++;
	}
	return f;
}

template<class Iterator>
int32_t read_int32(Iterator& first, const Iterator& last)
{
	int32_t i;
	uint8_t *fptr=reinterpret_cast<uint8_t*>(&i);
	ssize_t idx = 3;
	while (first != last && idx>=0) {
		fptr[idx--]=*first++;
	}
	return i;
}
}

template<class Iterator>
void OSCReceiver::process_data(Iterator& first, const Iterator& last)
{
	std::string name = read_string(first, last);
	if (name.empty()) return;
	log[log::verbose_debug] << "Found name " << name;
	if (name == "#bundle") {
		read_timestamp(first, last);
		while (first!=last) {
			int32_t size = read_size(first, last);
			log[log::verbose_debug] << "Element of size " << size;
			auto last2 = std::min(first+size,last);
			process_data(first,last2);
			first = last;
		}
	} else {
		std::string type = read_string(first,last);
		log[log::verbose_debug] << "Found types: " << type;
		if (type.size() < 2) {first=last;return;}
		if (type[0]!=',') {first=last;return;}
		std::vector<event::pBasicEvent> events;
		for (auto it=type.begin()+1; it!=type.cend();++it) {
			switch (*it) {
				case 'f': {
					float f = read_float(first,last);
					events.push_back(make_shared<event::EventDouble>(static_cast<double>(f)));
					log[log::verbose_debug] << "Float value: " << f;
				}; break;
				case 'i': {
					int32_t i = read_int32(first,last);
					events.push_back(make_shared<event::EventInt>(i));
					log[log::verbose_debug] << "Int value: " << i;
				}; break;
				default:
					{first=last;return;}
			}
		}
		if (events.empty()) return;
		if (events.size()==1) {
			emit_event(name, events[0]);
		} else {
			emit_event(name,make_shared<event::EventVector>(std::move(events)));
		}
	}
}

void OSCReceiver::run()
{
//	IO_THREAD_PRE_RUN
	//socket_.reset(new asio::ASIOUDPSocket(log, get_this_ptr(),port_));
	log[log::info] << "Initializing socket of type '"<< socket_type_ << "'";
	socket_ = core::DatagramSocketGenerator::get_instance().generate(socket_type_,log,"");
	log[log::info] << "Binding socket";
	if (!socket_->bind("",port_)) {
		log[log::fatal] << "Failed to bind socket!";
		request_end(core::yuri_exit_interrupted);
		return;
	}
	log[log::info] << "Socket initialized";
	std::vector<uint8_t> buffer(65536);
	ssize_t read_bytes=0;
	while(still_running()) {
		if (!socket_->data_available()) {
			sleep(10_ms);
			continue;
		} else {
			log[log::verbose_debug] << "reading data";
			read_bytes = socket_->receive_datagram(&buffer[0],buffer.size());
			if (read_bytes > 0) {
				log[log::verbose_debug] << "Read " << read_bytes << " bytes";
				auto first = buffer.begin();
				process_data(first, first + read_bytes);
			}
		}

	}
//	IO_THREAD_POST_RUN
	log[log::info] << "QUIT";
}
bool OSCReceiver::set_param(const core::Parameter& param)
{
	if (param.get_name() == "socket_type") {
		socket_type_ = param.get<std::string>();
	} else return core::IOThread::set_param(param);
	return true;
}

} /* namespace osc_receiver */
} /* namespace yuri */
