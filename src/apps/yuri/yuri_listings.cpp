/*!
 * @file 		yuri_listings.cpp
 * @author 		Zdenek Travnicek
 * @date 		9.11.2014
 * @copyright	Institute of Intermedia, CTU in Prague, 2014
 * 				CESNET z.s.p.o. 2014
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 */


#include "yuri_listings.h"
#include "yuri/event/BasicEvent.h"
#include "yuri/core/thread/IOThreadGenerator.h"
#include "yuri/core/socket/DatagramSocketGenerator.h"
#include "yuri/core/socket/StreamSocketGenerator.h"
#include "yuri/core/thread/ConverterRegister.h"
#include "yuri/core/pipe/PipeGenerator.h"
#include "yuri/core/thread/InputRegister.h"

#include "yuri/event/BasicEventConversions.h"

#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/compressed_frame_params.h"
#include "yuri/core/frame/raw_audio_frame_params.h"
#include "yuri/core/utils/string_generator.h"

#include <map>
#include <vector>
namespace yuri {
namespace app {


void list_registered_items(yuri::log::Log& l_, const std::string& list_what, int verbosity)
{
	if (iequals(list_what,"formats")) list_formats(l_, verbosity);
	else if (iequals(list_what,"datagram_sockets") || iequals(list_what,"datagram")) list_dgram_sockets(l_, verbosity);
	else if (iequals(list_what,"stream_sockets") || iequals(list_what,"stream")) list_stream_sockets(l_, verbosity);
	else if (iequals(list_what,"functions")) list_functions(l_, verbosity);
	else if (iequals(list_what,"converters")) list_converters(l_, verbosity);
	else if (iequals(list_what,"pipes")) list_pipes(l_, verbosity);
	else if (iequals(list_what,"input")) list_inputs(l_, verbosity);
	else if (iequals(list_what,"specifiers")) list_specifiers(l_, verbosity);
	else list_registered(l_, verbosity);
}


// Helper functions
namespace {

template<class T, class Stream>
void print_array(Stream& a, const std::vector<T>& data, const std::string& title)
{
	if (data.size()) {
		a << "\t"<< title << ":";
		for (const auto& sn: data) {
			a << " " << sn;
		}
	}
}

template<class T>
void print_array(yuri::log::Log& l_, const std::vector<T>& data, const std::string& title)
{
	auto a = l_[yuri::log::info];
	print_array(a, data, title);
}

using namespace yuri;
std::map<event::event_type_t, std::string> event_names=
{
		{event::event_type_t::bang_event, 		"BANG"},
		{event::event_type_t::boolean_event, 	"boolean"},
		{event::event_type_t::integer_event, 	"integer"},
		{event::event_type_t::double_event, 	"double"},
		{event::event_type_t::string_event, 	"string"},
		{event::event_type_t::time_event, 		"time"},
		{event::event_type_t::vector_event, 	"vector"},
		{event::event_type_t::dictionary_event,	"dictionary"},
		{event::event_type_t::duration_event,	"duration"},
		{event::event_type_t::undetermined_event,"undetermined"},

};
const std::string unknown_event_name {"??"};
const std::string& event_type_name(event::event_type_t e)
{
	auto it = event_names.find(e);
	if (it == event_names.end()) return unknown_event_name;
	return it->second;
}
const std::string unknown_format = "Unknown";
}


const std::string& get_format_name_no_throw(yuri::format_t fmt) {
	try {
		return yuri::core::raw_format::get_format_name(fmt);
	}
	catch(std::exception&){}
	try {
		return yuri::core::compressed_frame::get_format_name(fmt);
	}
	catch(std::exception&){}
	return unknown_format;
}


std::string param_value(const yuri::event::pBasicEvent& event) {

    if (event->get_type() == event::event_type_t::boolean_event) {
        return yuri::event::lex_cast_value<bool>(event) ? "True" : "False";
    } else if (event->get_type() == event::event_type_t::vector_event) {
        const auto val = std::dynamic_pointer_cast<event::EventVector>(event);
        std::string value = "[";
        for (const auto &ev: *val) {
            if (value.size() > 1) {
                value = value + ", ";
            }
            value = value + param_value(ev);
        }
        value = value + "]";
        return value;
    } else if (event->get_type() == event::event_type_t::dictionary_event){
        return "DICTIONARY VALUES NOT IMPLEMENTED";
    } else {
        return event::lex_cast_value<std::string>(event);
    }
}

void list_params(yuri::log::Log& l_, const yuri::core::Parameters& params, int /* verbosity */)
{
	using namespace yuri;
	for (const auto& p: params) {
		const auto& param = p.second;
		const std::string& pname = param.get_name();
		if (pname[0] != '_') {
		    try {
                const auto value = param_value(param.get_value());
                l_[log::info] << "\t\t"
                              << std::setfill(' ') << std::left << std::setw(20)
                              << (pname + ": ")
                              << std::right << std::setw(10) << value;
                const std::string &d = param.get_description();
                if (!d.empty()) {
                    l_[log::info] << "\t\t\t" << d;
                }
            }
		    catch (const std::exception& e) {
		        l_[log::warning] << "\t\t"
                              << std::setfill(' ') << std::left << std::setw(20)
                              << (pname + ": ")
                              << "FAILED TO DISPLAY VALUE (" << e.what() << ")";
		    }
		}
	}
}



void list_registered(yuri::log::Log& l_, int verbosity)
{
	using namespace yuri;
	if (verbosity>=0) l_[log::info]<<"List of registered objects:" ;
	auto& generator = yuri::IOThreadGenerator::get_instance();
	for (const auto& name: generator.list_keys()) {
		if (verbosity < 0) {
			l_[log::info] << name;
		} else {
			l_[log::info] /*<< "Module: "*/ << "..:: " << name << " ::..";
			const auto& params = generator.configure(name);
			const std::string& desc = params.get_description();
			if (!desc.empty()) {
				l_[log::info] << "\t" << desc;
			}
			list_params(l_, params);
			l_[log::info];
		}
	}

}

void list_single_class(yuri::log::Log& l_, const std::string& name, int /* verbosity */)
{
	using namespace yuri;
//	if (verbosity>=0)
	auto& generator = yuri::IOThreadGenerator::get_instance();
	if (!generator.is_registered(name)) {
		l_[log::fatal]<<"Class " << name << "is not registered";
	} else {
		const auto& params = generator.configure(name);
		l_[log::info]<<"Class " << name <<":";
		const std::string& desc = params.get_description();
		if (!desc.empty()) {
			l_[log::info] << "\t" << desc;
		}
		list_params(l_, params);

	}
}

void list_formats(yuri::log::Log& l_, int /* verbosity */)
{
	using namespace yuri;
	l_[log::info] << "List of registered formats:";
	l_[log::info] << "+++ RAW FORMATS +++";
	for (const auto& fmt: core::raw_format::formats())
	{
		const auto& info = fmt.second;
		l_[log::info] << "\"" << info.name << "\"";
		print_array(l_, info.short_names, "Short names");
		auto a = l_[log::info];
		a << "\tPlanes " << info.planes.size();
		for (const auto& pi: info.planes) {
			float bps = static_cast<float>(pi.bit_depth.first)/pi.bit_depth.second;
			a << ", " << pi.components << ": " << bps << "bps";
		}
	}

	l_[log::info] << "\t";
	l_[log::info] << "+++ COMPRESSED FORMATS +++";
	for (const auto& fmt: core::compressed_frame::formats())
	{
		const auto& info = fmt.second;
		l_[log::info] << "\"" << info.name << "\"";
		print_array(l_, info.short_names, "Short names");
		print_array(l_, info.mime_types, "Mime types");
		if (!info.fourcc.empty()) {
			l_[log::info] << "FOURCC: " << info.fourcc;
		}
	}

	l_[log::info] << "\t";
	l_[log::info] << "+++ RAW AUDIO FORMATS +++";
	for (const auto& fmt: core::raw_audio_format::formats())
	{
		const auto& info = fmt.second;
		l_[log::info] << "\"" << info.name << "\"";
		print_array(l_, info.short_names, "Short names");

		l_[log::info] << "Bits per sample: " << info.bits_per_sample;
		l_[log::info] << "Endianness: " << (info.little_endian?"little":"big");
	}
}
void list_dgram_sockets(yuri::log::Log& l_, int /* verbosity */)
{
	using namespace yuri;
	l_[log::info] << "List of registered datagram_socket implementations:";
	const auto& reg = core::DatagramSocketGenerator::get_instance();
	for (const auto& sock: reg.list_keys())
	{
		l_[log::info] << sock;

	}
}
void list_stream_sockets(yuri::log::Log& l_, int /* verbosity */)
{
	using namespace yuri;
	l_[log::info] << "List of registered datagram_socket implementations:";
	const auto& reg = core::StreamSocketGenerator::get_instance();
	for (const auto& sock: reg.list_keys())
	{
		l_[log::info] << sock;

	}
}

void list_functions(yuri::log::Log& l_, int /* verbosity */)
{
	using namespace yuri;
	l_[log::info] << "List of registered event functions:";
	auto& reg = event::EventFunctionsFactory::get_instance();
	std::map<std::string, std::vector<std::string>> signatures;
	for (const auto& func: reg.get_map())
	{
		const event::event_function_record_t& rec = func.second;
		//l_[log::info] << func.first;
		std::stringstream sstr;

		sstr << func.first << "(";
		bool first = true;
		for (const auto& x: rec.param_types) {
			if (first) {first = false;}
			else sstr << ", ";
			sstr << event_type_name(x);
		}
		sstr << ") -> " << event_type_name(rec.return_type);
		signatures[func.first].emplace_back(sstr.str());
	}
	for (const auto& name: signatures) {
		l_[log::info] << name.first;
		for (const auto& sig: name.second) {
			l_[log::info] << "\t" << sig;
		}
	}
}

void list_pipes(yuri::log::Log& l_, int verbosity)
{
	using namespace yuri;
	l_[log::info] << "List of registered pipe classes:";
	const auto& generator = core::PipeGenerator::get_instance();
	for (const auto& name: generator.list_keys()) {
		if (verbosity < 0) {
			l_[log::info] << name;
		} else {
			l_[log::info] /*<< "Module: "*/ << "..:: " << name << " ::..";
			const auto& params = generator.configure(name);
			const std::string& desc = params.get_description();
			if (!desc.empty()) {
				l_[log::info] << "\t" << desc;
			}
			list_params(l_, params);
			l_[log::info];
		}
	}
}
void list_converters(yuri::log::Log& l_, int /* verbosity */)
{
	using namespace yuri;
	const auto& conv = core::ConverterRegister::get_instance();
	const auto& keys = conv.list_keys();
	for (const auto& k: keys) {
		l_[log::info] << get_format_name_no_throw(k.first) << " -> " << get_format_name_no_throw(k.second);
		const auto& details = conv.find_value(k);
		for (const auto& d: details) {
			l_[log::info] << "\t\t" << d.first << ", priority: " << d.second;
		}
	}
}
void list_inputs(yuri::log::Log& l_, int /* verbosity */)
{
	using namespace yuri;
	const auto& conv = core::InputRegister::get_instance();
	const auto& keys = conv.list_keys();
	for (const auto& k: keys) {
		l_[log::info] << k;
	}
}

void list_input_class(yuri::log::Log& l_, const std::string& name, int /*verbosity*/)
{
	const auto& conv = core::InputRegister::get_instance();
	if (name == "all") {
		l_[log::info] << "Enumerating all input classes";
		const auto& keys = conv.list_keys();
		for (const auto& k: keys) {
			l_[log::info] << "input class: " << k;
			list_input_class(l_, k);
		}
		return;
	}

	try {
		auto enumerate = conv.find_value(name);
		auto devices = enumerate();
		l_[log::info] << "Found " << devices.size() << " devices";
		for (const auto d: devices) {
			l_[log::info] << "\tDevice " << d.device_name << " with " << d.configurations.size() << " configurations";
			print_cfgs(l_, log::info, d);
		}

	}
	catch (...) {
		l_[log::info] << "No input thread found for " << name;
	}
}


void list_specifiers(yuri::log::Log& l_, int )
{
	const auto& spec = core::utils::enumerate_string_generator_specifiers();
	l_[log::info] << "Valid string generator specifiers:";
	for (const auto& s: spec) {
		l_[log::info] << "\t%" << s.placeholder << "\t" << s.description << (s.accepts_width?" (*)":"");
	}
	l_[log::info] << "Specifiers marked with (*) also accept extended syntax in form %[[0]width]X, for example: %05i";

}

}
}


