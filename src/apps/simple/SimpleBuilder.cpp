/*
 * SimpleBuilder.cpp
 *
 *  Created on: 11. 1. 2015
 *      Author: neneko
 */

#include "SimpleBuilder.h"
#include "yuri/core/thread/builder_utils.h"
#include <boost/regex.hpp>
#include <stdexcept>
#include <algorithm>
namespace yuri {

namespace simple {

namespace {

std::string gen_name(const std::string& type, const std::string& cls, int i)
{
	return type+"_"+cls+"_"+std::to_string(i);
}

struct parsed_parameter_t {
	std::string class_name;
	std::string name;
	core::Parameters params;
};
/*!
 * Parses an argument
 *
 * argument should be either just node name, or in the form:
 * name[param=value,param=value]
 *
 * @param log
 * @param arg
 * @param i
 * @return
 */
parsed_parameter_t parse_argument(log::Log& log, const std::string& arg, int i)
{
	parsed_parameter_t node;
	boost::regex node_expr ("([a-zA-Z0-9_-]+)(\\[[^]]+\\])?");
	boost::smatch what;
	auto start = arg.cbegin();
	const auto end = arg.cend();

	if (!regex_search(start, end, what, node_expr, boost::match_default)) {
		throw std::runtime_error("Failed to parse "+arg);
	}
	node.class_name = std::string(what[1].first, what[1].second);
	if (std::distance(what[2].first, what[2].second) > 2) {
		std::map<std::string,event::pBasicEvent> var_cache;

		// supported param values:
		//				"abcahdkashdlka ,)(0 "
		//				abc
		//				abc(asd)()(ddd, 3, s)

		boost::regex param_line("([^=]+)=" // Parameter name
				"([^\",]"  "(([^,(]*(\\([^)]*\\))*)*)|" // Unqouted values (with optional brackets)
				"(\"[^\"]*\"))(,)?"); // Quoted values

		boost::sregex_iterator it(what[2].first+1, what[2].second-1, param_line, boost::match_default);
		boost::sregex_iterator it_end;
		while (it != it_end) {
			const auto& res = *it;
			const auto param_name  =  std::string(res[1].first,res[1].second);
			const auto param_value = std::string(std::string(res[2].first,res[2].second));
			++it;
			if (auto parsed_event = event::BasicEventParser::parse_expr(log, param_value, var_cache)) {
				node.params[param_name]= parsed_event;
			} else {
				node.params[param_name] = param_value;
			}
			var_cache[param_name]=node.params[param_name].get_value();
		}
	}
	node.name = gen_name("node", node.class_name, i);
	return node;
}

}



SimpleBuilder::SimpleBuilder(const log::Log& log_, core::pwThreadBase parent, const std::vector<std::string>& argv)
:GenericBuilder(log_, parent,"simple")
{
	core::builder::load_builtin_modules(log);

	core::node_map nodes;
	core::link_map links;

	std::string last;
	int i = 0;
	parsed_parameter_t link_info{"single", {}, {}};
	for (const auto& s: argv) {
		if (s.substr(0,2) == "-p") {
			link_info = parse_argument(log, s.substr(2), ++i);
			continue;
		}
		auto param = parse_argument(log, s, ++i);
		core::node_record_t node = {std::move(param.name), std::move(param.class_name), std::move(param.params), {}};
		nodes[node.name]=node;//{name, node_cls, {}, {}};
		if (!last.empty()) {
			auto link_name = gen_name("link","single",i);
			log[log::info] << "link " << link_name << " from " << last << " to " << node.name;
			links[link_name]={link_name, link_info.class_name, link_info.params, last, node.name, 0, 0, {}};
		}

		last = node.name;
	}
	set_graph(nodes, links);
}


}
}


