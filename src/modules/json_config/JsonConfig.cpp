/*!
 * @file 		JsonConfig.cpp
 * @author 		<Your name>
 * @date		22.01.2015
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#include "JsonConfig.h"
#include "yuri/core/Module.h"
//#include "yuri/event/EventHelpers.h"
namespace yuri {
namespace json_config {


IOTHREAD_GENERATOR(JsonConfig)

MODULE_REGISTRATION_BEGIN("json_config")
		REGISTER_IOTHREAD("json_config",JsonConfig)
MODULE_REGISTRATION_END()

core::Parameters JsonConfig::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("JsonConfig");
	p["filename"]["Config filename"]="";
	p["readonly"]["Do not write to the disk"]=false;
	return p;
}


JsonConfig::JsonConfig(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,1,1,std::string("json_config")),
BasicEventConsumer(log),
BasicEventProducer(log),readonly_{false}
{
	IOTHREAD_INIT(parameters)
	if (filename_.empty()) {
		throw exception::InitializationFailed("No config file specified!");
	}
}

JsonConfig::~JsonConfig() noexcept
{
}

namespace {

event::pBasicEvent parse_value(const Json::Value& node)
{
	switch (node.type()) {
		case Json::booleanValue:
			return std::make_shared<event::EventBool>(node.asBool());
		case Json::intValue:
			return std::make_shared<event::EventInt>(node.asInt());
		case Json::uintValue:
			return std::make_shared<event::EventInt>(node.asUInt());
		case Json::realValue:
			return std::make_shared<event::EventDouble>(node.asDouble());
		case Json::stringValue:
			return std::make_shared<event::EventString>(node.asString());
		default:
			break;
	}
	return {};
}

event_map parse_tree(log::Log& log, const std::string& prefix, const Json::Value root)
{
	log[log::info] << root.type();

	switch (root.type()) {
		case Json::booleanValue:
		case Json::intValue:
		case Json::uintValue:
		case Json::realValue:
		case Json::stringValue:
			return {{prefix, parse_value(root)}};
		case Json::arrayValue:
		{
			log[log::info] << "values: " << root.size();
			std::vector<event::pBasicEvent> events;
			for (const auto& node: root) {
				events.push_back(parse_value(node));
			}
			return {{prefix, std::make_shared<event::EventVector>(std::move(events))}};
		}

		case Json::objectValue:
			{
				std::map<std::string, event::pBasicEvent> data;
				const auto& names = root.getMemberNames();
				for (const auto& name: names) {
					log[log::info] << name;
					const auto new_prefix = /*prefix.empty()?name:(*/prefix+'/'+name;//);
					auto new_data = parse_tree(log, new_prefix, root[name]);
					log[log::info] << "got " << new_data.size() << " values";
					data.insert(new_data.begin(), new_data.end());
				}
				return data;
			}
		default:
			break;

	}
	log[log::warning] << "Unsupported node";
	return {};
}

event_map parse_config(log::Log& log, const std::string& filename)
{
	std::ifstream file(filename, std::ios::in);

	if (file.is_open()) {
		log[log::info] << "File opened";
		Json::Reader reader;
		Json::Value root;
		reader.parse(file, root);
		log[log::info] << "Parsed";
		return parse_tree(log, "", root);
	}
	return {};
}

Json::Value get_value(log::Log& log, const event::pBasicEvent& event)
{
	switch (event->get_type()) {
		case event::event_type_t::boolean_event:
			return Json::Value{event::get_value<event::EventBool>(event)};
		case event::event_type_t::integer_event:
			return Json::Value{event::lex_cast_value<Json::Int>(event)};
		case event::event_type_t::double_event:
			return Json::Value{event::lex_cast_value<double>(event)};
		case event::event_type_t::string_event:
			return Json::Value{event::get_value<event::EventString>(event)};
		case event::event_type_t::vector_event:
			{
				auto eval = event::get_value<event::EventVector>(event);
				auto jval = Json::Value(Json::arrayValue);
				for (const auto& v: eval) {
					auto new_val = get_value(log, v);
					if (new_val.type() == Json::nullValue) continue;
					jval.append(new_val);
				}
				return jval;
			}
		default:
			break;
	}
	return Json::Value{Json::nullValue};
}

void store_value(Json::Value& node, const std::string& name, const Json::Value& value)
{
	if (name.empty() || name[0] != '/') return;
	auto idx = name.find('/',1);
	if (idx==std::string::npos) {
		node[name.substr(1)]=value;
	} else {
		const std::string subname = std::string{name.substr(1,idx-1)};
		const std::string lastname = std::string{name.substr(idx)};
		if (!node.isMember(subname)) {
			Json::Value v{Json::objectValue};
			node[subname]=v;
		}
		store_value(node[subname], lastname, value);
	}

}

void dump_config(log::Log& log, const event_map& cfg, const std::string& filename)
{
	std::ofstream file(filename, std::ios::out|std::ios::trunc);
	if (file.is_open()) {
		Json::Value root{Json::objectValue};
		Json::StyledStreamWriter writer;

		for (const auto& val: cfg) {
			auto name = val.first;
			if (name.empty() || name[0] != '/') continue;
			auto value = get_value(log, val.second);
			if (value.type() == Json::nullValue) continue;
			store_value(root, name, value);
		}
		writer.write(file, root);
	} else {
		log[log::warning] << "Failed to open output file";
	}
}

}

void JsonConfig::run()
{
	event_map_ = parse_config(log, filename_);
	log[log::info] << "got " << event_map_.size() << " values";
	for (const auto& v: event_map_) {
		emit_event(v.first, v.second);
	}
	while(still_running()) {
		wait_for_events(get_latency());
		process_events();
	}
	if (!readonly_) {
        dump_config(log, event_map_, filename_);
    }

}
bool JsonConfig::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			(filename_, "filename")
            (readonly_, "readonly"))
		return true;
	return core::IOThread::set_param(param);
}

bool JsonConfig::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	event_map_[event_name]=event;
	emit_event(event_name, event);
	return false;
}
} /* namespace json_config */
} /* namespace yuri */
