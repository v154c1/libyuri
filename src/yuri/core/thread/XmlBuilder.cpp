/*!
 * @file 		XmlBuilder.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		15.9.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "XmlBuilder.h"
#include "yuri/core/thread/IOThreadGenerator.h"
#include "yuri/core/pipe/PipeGenerator.h"
#include "yuri/core/utils/ModuleLoader.h"
#include "yuri/core/utils/assign_parameters.h"
#include "builder_utils.h"
#define TIXML_USE_STL
#ifdef YURI_WIN
#pragma warning( push )
// Disable "declaration of xxx hides class member
#pragma warning( disable : 4458)
#endif
#include "yuri/core/tinyxml/tinyxml.h"
#ifdef YURI_WIN
#pragma warning(pop)
#endif
#ifdef YURI_CYGWIN
#include <cstdlib>
#endif
namespace yuri {
namespace core {


IOTHREAD_GENERATOR(XmlBuilder)

MODULE_REGISTRATION_BEGIN("xml_builder")
		REGISTER_IOTHREAD("xml_builder", XmlBuilder)
MODULE_REGISTRATION_END()

namespace {

const std::string root_tag 			{"app"};
const std::string module_tag 		{"module"};
const std::string module_dir_tag 	{"module_dir"};
const std::string variable_tag 		{"variable"};
const std::string general_tag 		{"general"};
const std::string parameter_tag		{"parameter"};
const std::string node_tag 			{"node"};
const std::string link_tag 			{"link"};
const std::string description_tag 	{"description"};
const std::string event_tag			{"event"};

const std::string path_attrib		{"path"};
const std::string name_attrib		{"name"};
const std::string class_attrib		{"class"};
const std::string description_attrib{"description"};
const std::string source_attrib		{"source"};
const std::string target_attrib		{"target"};


template<typename T, typename U>
typename std::enable_if<std::is_convertible<U, T>::value,bool>::type
load_value(TiXmlElement* element, const std::string& name, T& value, const U& def) {
	if (element->QueryValueAttribute(name, &value) == TIXML_SUCCESS) return true;
	value = def;
	return true;
}


#ifdef YURI_CYGWIN
// For some reason, there's no std::stoll under cygwin...
	long long stoll(const std::string& str) { return std::atol(str.c_str()); }
#else
	using std::stoll;
#endif

}
struct XmlBuilder::builder_pimpl_t{
	builder_pimpl_t(log::Log& log_, XmlBuilder& builder):log(log_),builder(builder){}
	void load_xml_file(const std::string&);
	void load_xml_text(const std::string&);
	void process_document();

	void process_modules();
	void process_module_dirs();
	void process_argv(const std::vector<std::string>&);
	void process_variables();
	Parameters process_general();
	void process_nodes();
	void process_links();

	void process_routing();

	void load_builtin_modules();

	event::pBasicEvent parse_expression(const std::string expression);
	Parameters parse_parameters(const TiXmlElement* element);

	void verify_node_class(const std::string& class_name);
	void verify_link_class(const std::string& class_name);
	void verify_links();
	void add_variable_routes();

	pIOThread get_node(const std::string& name);
	log::Log& log;
	XmlBuilder& builder;
	std::unique_ptr<TiXmlDocument> doc;
	TiXmlElement* root {nullptr};
	std::string name;
	std::string description;
//	std::vector<std::string> routing_info;


	std::vector<std::string> module_dirs;
	Parameters argv;
	Parameters variables;
	std::map<std::string, event::pBasicEvent> input_events;
	std::map<std::string, node_record_t> nodes;
	std::map<std::string, link_record_t> links;
	std::string routing;
};

#define VALID(x,msg) if (!(x)) { log[log::error] << msg; throw exception::InitializationFailed(msg); }
#define VALID_XML(x) VALID(x,"Invalid XML")

void XmlBuilder::builder_pimpl_t::load_xml_file(const std::string& file)
{

	doc.reset(new TiXmlDocument());
	VALID(doc->LoadFile(file),"Failed to load file " + file);
	process_document();
}

void XmlBuilder::builder_pimpl_t::load_xml_text(const std::string& text)
{

	doc.reset(new TiXmlDocument());
//	VALID(doc->LoadFile(file),"Failed to load file " + file);
	doc->Parse(text.c_str(), 0, TIXML_ENCODING_UTF8);
	process_document();
}


void XmlBuilder::builder_pimpl_t::process_document()
{
	root = doc->RootElement();
	VALID_XML(root)
	load_value(root, name_attrib, name, "yuri2.8");
	TiXmlElement * node = nullptr;
	if((node = dynamic_cast<TiXmlElement*>(root->IterateChildren(description_tag, node)))) {
		const char* text = node->GetText(); // GetText may return nullptr and basic_string::operator= doesn't accept nullptr...
		if (text) description = text;
	}
	process_routing();
}

void XmlBuilder::builder_pimpl_t::process_modules()
{
//	log[log::verbose_debug] << "Loading <module>s";
	TiXmlElement * node = nullptr;
	std::vector<std::string> modules;
	VALID_XML(root);
	while((node = dynamic_cast<TiXmlElement*>(root->IterateChildren(module_tag, node)))) {
		std::string path;
		if (node->QueryValueAttribute(path_attrib, &path)!=TIXML_SUCCESS) continue;
		modules.push_back(std::move(path));
	}
	builder::load_modules(log, modules);
}

void XmlBuilder::builder_pimpl_t::process_module_dirs()
{
//	log[log::verbose_debug] << "Loading <module_dir>s";
	TiXmlElement * node = nullptr;
	std::vector<std::string> module_dirs_local;
	while((node = dynamic_cast<TiXmlElement*>(root->IterateChildren(module_dir_tag, node)))) {
		std::string path;
		if (node->QueryValueAttribute(path_attrib, &path)!=TIXML_SUCCESS) continue;
		module_dirs_local.push_back(std::move(path));
	}
	for (const auto& m: module_dirs_local) {
		builder::load_module_dir(log, m);
	}
}

void XmlBuilder::builder_pimpl_t::load_builtin_modules()
{
	builder::load_builtin_modules(log);
}

void XmlBuilder::builder_pimpl_t::process_argv(const std::vector<std::string>& var)
{
	for (const std::string& param_pair: var) {
		auto idx = param_pair.find('=');
		if (idx == param_pair.npos) continue;
		const std::string& val = param_pair.substr(idx+1);
		Parameter p(param_pair.substr(0,idx));
		/*if (event::pBasicEvent event = event::BasicEventParser::parse_expr(log, val, input_events)) {
			p.set_value(event);
			input_events[p.get_name()]=event;
		} else {
			p=val;
			input_events[p.get_name()]=val;
		}*/
		auto event = parse_expression(val);
		p.set_value(event);
		input_events[p.get_name()]=event;
		argv[p.get_name()]=std::move(p);

	}
}

void XmlBuilder::builder_pimpl_t::process_variables()
{
	TiXmlElement * node = nullptr;
	for (const auto& av: argv) {
		input_events[av.first]=av.second.get_value();
	}
	while((node = dynamic_cast<TiXmlElement*>(root->IterateChildren(variable_tag, node)))) {
		std::string node_name;
		if (node->QueryValueAttribute(name_attrib, &node_name)!=TIXML_SUCCESS) continue;
//		log[log::info] << "Checking variable " << name;
		std::string desc;
		node->QueryValueAttribute(description_attrib, &desc);// This is optional
		auto it = input_events.find(node_name);
		if (it != input_events.end()){
			variables[node_name][desc].set_value(it->second);
		} else {
			const char* text = node->GetText();
			if (!text) text = "";
			variables[node_name][desc]=parse_expression(text);
			input_events[node_name]=variables[node_name].get_value();
		}
	}
//	for (const auto&p: variables) {
//		log[log::info] << "Found variable: " << p.first << ": " << p.second.get<std::string>();
//	}
}
Parameters XmlBuilder::builder_pimpl_t::process_general()
{
	TiXmlElement* node = dynamic_cast<TiXmlElement*>(root->IterateChildren(general_tag, nullptr));
	return parse_parameters(node);
}

event::pBasicEvent XmlBuilder::builder_pimpl_t::parse_expression(const std::string expression)
{
	if (event::pBasicEvent event = event::BasicEventParser::parse_expr(log, expression, input_events)){
//		log[log::info] << "Got event" ;
		return event;
//		p.set_value(event);
	} else {
//		log[log::info] << "No event, assuming a string" ;
		return std::make_shared<event::EventString>(expression);
	}
}
Parameters XmlBuilder::builder_pimpl_t::parse_parameters(const TiXmlElement* element)
{
	Parameters params;
	if (element) {
		const TiXmlElement * node = nullptr;
		while((node = dynamic_cast<const TiXmlElement*>(element->IterateChildren(parameter_tag, node)))) {
			std::string node_name;
			if (node->QueryValueAttribute(name_attrib, &node_name)!=TIXML_SUCCESS) continue;
			const char* text = node->GetText();
			if (!text) text = "";
			params[node_name]=parse_expression(text);
		}
	}
	return params;
}
void XmlBuilder::builder_pimpl_t::process_nodes()
{
	TiXmlElement * node = nullptr;
	while((node = dynamic_cast<TiXmlElement*>(root->IterateChildren(node_tag, node)))) {
		node_record_t record;
		VALID_XML(node->QueryValueAttribute(name_attrib, &record.name)==TIXML_SUCCESS)
		VALID_XML(node->QueryValueAttribute(class_attrib, &record.class_name)==TIXML_SUCCESS)
		record.parameters = parse_parameters(node);
		builder::verify_node_class(record.class_name);
		if (nodes.find(record.name) != nodes.end()) {
		    throw exception::InitializationFailed("Duplicate node name " + record.name);
		}
		nodes[record.name]=std::move(record);
	}
}
void XmlBuilder::builder_pimpl_t::process_links()
{
	TiXmlElement * node = nullptr;
	while((node = dynamic_cast<TiXmlElement*>(root->IterateChildren(link_tag, node)))) {
		link_record_t record;
		VALID_XML(node->QueryValueAttribute(name_attrib, &record.name)==TIXML_SUCCESS)
		VALID_XML(node->QueryValueAttribute(class_attrib, &record.class_name)==TIXML_SUCCESS)
		std::string source, target;
		VALID_XML(node->QueryValueAttribute(source_attrib, &source)==TIXML_SUCCESS)
		VALID_XML(node->QueryValueAttribute(target_attrib, &target)==TIXML_SUCCESS)
		auto idx = source.find(':');
		VALID(idx!=std::string::npos,"Malformed source specification in source for link " + record.name)
		record.source_node = source.substr(0,idx);
		record.source_index = stoll(source.substr(idx+1));
		idx = target.find(':');
		VALID(idx!=std::string::npos,"Malformed target specification in source for link " + record.name)
		record.target_node = target.substr(0,idx);
		record.target_index = stoll(target.substr(idx+1));
		record.parameters = parse_parameters(node);
		builder::verify_link_class(record.class_name);
		if (links.find(record.name) != links.end()) {
		    throw exception::InitializationFailed("Duplicate link name " + record.name);
		}
		links[record.name]=std::move(record);
	}
}
void XmlBuilder::builder_pimpl_t::process_routing()
{
	TiXmlElement * node = nullptr;
	while((node = dynamic_cast<TiXmlElement*>(root->IterateChildren(event_tag, node)))) {
		TiXmlNode* tnode = nullptr;
		while((tnode = node->IterateChildren(tnode))) {
			if (auto text_node = tnode->ToText()) {
				auto text = text_node->Value();
				if (text) routing.append(text);
			}
		}
	}
}

namespace {
std::string escape_quotes(std::string str)
{
//	std::string::size_type pos = 0;
//	while ((pos = str.find("\"", pos)) != std::string::npos) {
// 		The correct solution would be to escape the quote character,
//		but current parser doesn't support escape sequences. So let's simply delete all quotes.
//		str.insert(pos, "\\");
//		pos+=2;
//		str.erase(pos);
//	}
	return str;
}

}

void XmlBuilder::builder_pimpl_t::add_variable_routes()
{
	std::stringstream variable_routes_stream;
	for (const auto& param: variables) {
		variable_routes_stream << "route(pass(\""<<escape_quotes(param.second.get<std::string>()) << "\"))->@:"<<param.first << ";\n";
	}
	routing = variable_routes_stream.str() + routing;
}

void XmlBuilder::builder_pimpl_t::verify_links()
{
	using map_elem = std::pair<std::string, position_t>;
	std::map<map_elem, std::string> used_sources, used_targets;
	for (const auto& link: links) {
		const auto& src_node = link.second.source_node;
		const auto& tgt_node = link.second.target_node;

//		auto s = nodes.find(link.second.source_node);
//		auto t = nodes.find(link.second.target_node);
		VALID(is_special_link_target(src_node) || contains(nodes, src_node)
				,"Unknown source node " + link.second.source_node + " in link " +link.first)
		VALID(is_special_link_target(tgt_node) || contains(nodes, tgt_node)
				,"Unknown target node " + link.second.target_node + " in link " +link.first)
		map_elem selem {link.second.source_node, link.second.source_index};
		map_elem telem {link.second.target_node, link.second.target_index};
		auto s2 = used_sources.find(selem);
		auto t2 = used_targets.find(telem);
		VALID(s2 == used_sources.end() || link.second.source_index < 0, "Duplicate specification for source " + selem.first +":"+lexical_cast<std::string>(selem.second)+", specified in "+s2->second+" and "+link.first)
		VALID(t2 == used_targets.end() || link.second.target_index < 0, "Duplicate specification for target " + telem.first +":"+lexical_cast<std::string>(telem.second)+", specified in "+t2->second+" and "+link.first)
		used_sources[selem]=link.first;
		used_targets[telem]=link.first;
	}
}

#undef VALID
#undef VALID_XML

Parameters XmlBuilder::configure()
{
	Parameters p = IOThread::configure();
	p["filename"]["Path to  XML file."]="";
	p["run_limit"]["Runtime limit in seconds"]=0.0;
	p["variable_events"]["Send all variables as events at startup"]=true;
	return p;
}


XmlBuilder::XmlBuilder(const log::Log& log_, pwThreadBase parent, const Parameters& parameters)
:GenericBuilder(log_,parent,"XmlBuilder")
{
	pimpl_.reset(new builder_pimpl_t(log,*this));
	set_latency(10_ms);
//	pimpl_->load_file(filename_);
	pimpl_->load_xml_file(parameters["filename"].get<std::string>());

	pimpl_->process_modules();
	pimpl_->process_module_dirs();

	// Emulate argv from params
	std::vector<std::string> argv;
	for (const auto& param: parameters) {
		argv.push_back(param.first+"="+param.second.get<std::string>());
	}
	pimpl_->process_argv(argv);

	pimpl_->process_variables();
	Parameters general = pimpl_->process_general();
	IOTHREAD_INIT(general.merge(parameters)); //TODO Shouldn't this be the other way round? general won't be used at all if used like this...
	pimpl_->load_builtin_modules();
	pimpl_->process_nodes(); // TODO process all nodes
	pimpl_->process_links(); // TODO process all links
	pimpl_->verify_links();
	if (use_variable_events_) pimpl_->add_variable_routes();
	log[log::debug] << "File seems to be parsed successfully";
	set_graph(pimpl_->nodes, pimpl_->links, pimpl_->routing);

}
XmlBuilder::XmlBuilder(const log::Log& log_, pwThreadBase parent, const std::string& filename, const std::vector<std::string>& argv, bool parse_only, std::string xml_text)
:GenericBuilder(log_,parent,"XmlBuilder")
{
	pimpl_.reset(new builder_pimpl_t(log, *this));
	set_latency(10_ms);
	try {
		if (!xml_text.empty()) {
			filename_ = "none";
			pimpl_->load_xml_text(xml_text);
		} else {
			filename_ = filename;
			pimpl_->load_xml_file(filename_);
		}
		pimpl_->process_modules();
		pimpl_->process_module_dirs();
		pimpl_->process_argv(argv);
		pimpl_->process_variables();
		Parameters general = pimpl_->process_general();
		IOTHREAD_INIT(general);
	} catch (...) {
		if (!parse_only) throw;
	}
	pimpl_->load_builtin_modules();

	if (parse_only) return;
	pimpl_->process_nodes(); // TODO process all nodes
	pimpl_->process_links(); // TODO process all links
	pimpl_->verify_links();
	if (use_variable_events_) pimpl_->add_variable_routes();
	log[log::debug] << "File seems to be parsed successfully";
	set_graph(pimpl_->nodes, pimpl_->links, pimpl_->routing);
}

XmlBuilder::~XmlBuilder() noexcept {}


bool XmlBuilder::set_param(const Parameter& parameter)
{
	if (assign_parameters(parameter)
			(filename_, "filename")
			(max_run_time_, "run_limit", [](const core::Parameter& p){return 1_s * p.get<double>();})
			(use_variable_events_, "variable_events")
			)
	{
		return true;
	}
	IOThread::set_param(parameter);
	// Return always true so pass-through parameters work without warnings
	return true;
}

const std::string& XmlBuilder::get_app_name()
{
	return pimpl_->name;
}
const std::string& XmlBuilder::get_description()
{
	return pimpl_->description;
}
std::vector<variable_info_t> XmlBuilder::get_variables() const
{
	std::vector<variable_info_t> vars;
	for (const auto& v: pimpl_->variables) {
		const auto& var = v.second;
		vars.push_back({var.get_name(), var.get_description(), var.get<std::string>()});
	}
	return vars;
}

void XmlBuilder::run()
{
	start_time_ = timestamp_t{};
	GenericBuilder::run();
	log[log::info] << "Finishing run after " << (timestamp_t{} - start_time_);
	finish_all_threads();
}
bool XmlBuilder::step()
{
	if (max_run_time_ > 1_ms) {
		const auto& delta = timestamp_t{} - start_time_;
		if (delta > max_run_time_) {
			log[log::warning] << "Max run time reached ("<<delta<<"). Quitting";
			request_end(core::yuri_exit_interrupted);
		}
	}
	return GenericBuilder::step();
}

}
}

