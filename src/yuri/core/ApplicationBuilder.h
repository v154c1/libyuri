/*!
 * @file 		ApplicationBuilder.h
 * @author 		Zdenek Travnicek
 * @date 		28.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2010 - 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#ifndef APPLICATIONBUILDER_H_
#define APPLICATIONBUILDER_H_

#include "yuri/core/BasicIOThread.h"
//#include "yuri/config/Parameters.h"
#define TIXML_USE_STL
#include "tinyxml/tinyxml.h"
#include <map>

namespace yuri {

namespace core {

struct NodeRecord {
std::string name, type;
	Parameters params;
	std::map<std::string, std::string> variables;
	NodeRecord(std::string name, std::string type):name(name),type(type) {}
};

struct LinkRecord {
std::string name, source_node, target_node;
	int source_index, target_index;
	Parameters params;
	std::map<std::string, std::string> variables;
	LinkRecord(std::string name, std::string source_node,std::string target_node,
			int source_index, int target_index):
		name(name), source_node(source_node), target_node(target_node),
		source_index(source_index), target_index(target_index) {}
};

struct VariableNodeDependency{
	weak_ptr<NodeRecord> node;
	std::string parameter;
};
struct VariableLinkDependency{
	weak_ptr<LinkRecord> node;
	std::string parameter;
};
struct VariableRecord {
	std::string name, def, value;
	bool required;
	VariableRecord(std::string name, std::string def, bool required = false):name(name),def(def),value(def),required(required) {}
	VariableRecord():required(false) {}
	std::vector<shared_ptr<VariableNodeDependency> > node_dependencies;
	std::vector<shared_ptr<VariableLinkDependency> > linkdependencies;
};

class EXPORT ApplicationBuilder: public BasicIOThread {
public:
	ApplicationBuilder(log::Log &_log, pwThreadBase parent,std::string filename="", std::vector<std::string> argv=std::vector<std::string>());
	ApplicationBuilder(log::Log &_log, pwThreadBase parent, Parameters &params);
	virtual ~ApplicationBuilder();
	IO_THREAD_GENERATOR_DECLARATION
	static pParameters configure();

	bool load_file(std::string path);
	void run();
	pBasicIOThread get_node (std::string id);
	bool prepare_threads();
	bool find_modules();
	bool load_modules();
private:
	bool process_module_dir(TiXmlElement &element);
	bool process_module(TiXmlElement &element);
	bool process_node(TiXmlElement &element);
	bool process_link(TiXmlElement &element);
	bool process_variable(TiXmlElement &element);
	bool parse_parameters(TiXmlElement &element,Parameters &params);
	void clear_tree();
	bool check_variables();
	//bool check_links();


	bool prepare_links();
	bool spawn_threads();
	bool stop_threads();
	bool delete_threads();
	bool delete_pipes();
	bool step();

	void init();
	void init_local_params();
	void show_params(Parameters& _params,std::string prefix="\t\t");
	void fetch_tids();
	pParameters assign_variables(std::map<std::string, std::string> vars);
	void parse_argv(std::vector<std::string> argv);
	bool set_param(const core::Parameter& parameter);
private:
	std::string filename;
	std::string description;
	TiXmlDocument doc;

	bool document_loaded, threads_prepared, skip_verification;
	boost::posix_time::time_duration run_limit;
	boost::posix_time::ptime start_time;
	pParameters default_pipe_param;
	std::map<std::string,shared_ptr<NodeRecord> > nodes;
	std::map<std::string,shared_ptr<LinkRecord> > links;
	std::map<std::string,pBasicIOThread > threads;
	std::map<std::string,shared_ptr<BasicPipe> > pipes;
	std::map<std::string,shared_ptr<VariableRecord> > variables;
	std::vector<std::string> modules;
	std::vector<std::string> module_dirs;
	std::map<std::string,pid_t > tids;

};

}

}

#endif /* APPLICATIONBUILDER_H_ */
