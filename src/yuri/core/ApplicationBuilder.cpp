/*!
 * @file 		ApplicationBuilder.cpp
 * @author 		Zdenek Travnicek
 * @date 		28.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2010 - 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#include "ApplicationBuilder.h"
#include "yuri/core/RegisteredClass.h"
#include "yuri/core/BasicPipe.h"
#include <boost/lexical_cast.hpp>
#include <cassert>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
namespace yuri {

namespace core {

REGISTER("appbuilder",ApplicationBuilder)

using boost::lexical_cast;

IO_THREAD_GENERATOR(ApplicationBuilder)

pParameters ApplicationBuilder::configure()
{
	pParameters p = core::BasicIOThread::configure();
	(*p)["run_limit"]["Runtime limit in seconds"]=-1.0;
	(*p)["config"]["Config file"]="";
	(*p)["debug"]["Debug level (2 most verbose, -2 least verbose)"]="0";
	(*p)["show_time"]["show timestamps"]=true;
	(*p)["show_level"]["show debug levels"]=true;
	(*p)["use_colors"]["Use color output"]=true;
	(*p)["skip_verification"]["Skips verification for required variables. Should NOT be set unless you know what you're doing."]=false;
	return p;
}


ApplicationBuilder::ApplicationBuilder(log::Log &_log, pwThreadBase parent, Parameters &p)
	:BasicIOThread(_log,parent,0,0,"AppBuilder"),filename(""),
	document_loaded(false),threads_prepared(false),skip_verification(false),
	run_limit(boost::posix_time::pos_infin),start_time(boost::posix_time::not_a_date_time)

{
	default_pipe_param=BasicPipe::configure();
	params.merge(p);
	init();
	if (!skip_verification) {
		check_variables();
	}
}

ApplicationBuilder::ApplicationBuilder(log::Log &_log, pwThreadBase parent,std::string filename,std::vector<std::string> argv, bool skip)
	:BasicIOThread(_log,parent,0,0,"AppBuilder"),filename(filename),document_loaded(false),
	threads_prepared(false),skip_verification(skip),run_limit(boost::posix_time::pos_infin),
	start_time(boost::posix_time::not_a_date_time)

{
	default_pipe_param=BasicPipe::configure();
	pParameters p = configure();
	params.merge(*p);
	params["config"]=filename;
	params["skip_verification"]=skip;
	init();
	parse_argv(argv);
	if (!skip_verification) {
		check_variables();
	}
}

ApplicationBuilder::~ApplicationBuilder()
{
	clear_tree();
#ifdef __linux__
	std::string destdir =std::string("/tmp/")+lexical_cast<std::string>(getpid());
	boost::filesystem::path p(destdir);
	if (boost::filesystem::exists(p)) {
		boost::filesystem::remove_all(p);
	}
#endif
}

void ApplicationBuilder::run()
{
	//check_links();
	double limit = params["run_limit"].get<double>();

	if (limit<0) run_limit=boost::posix_time::pos_infin;
	else {
		run_limit = boost::posix_time::seconds(floor(limit))+
			boost::posix_time::seconds(floor(1e6*(limit-floor(limit))));
	}
	log[log::debug] << "Got limit " << limit << ", which was converted to " << run_limit <<"\n";
	start_time=boost::posix_time::microsec_clock::local_time();
	try {
		if (!prepare_threads()) throw (exception::InitializationFailed("Failed to prepare threads!"));
		log[log::debug] << "Threads prepared" <<"\n";
		if (!prepare_links()) throw (exception::InitializationFailed("Failed to prepare links!"));
		log[log::debug] << "Links prepared" <<"\n";
		if (!spawn_threads()) throw (exception::InitializationFailed("Failed to spawn threads!"));
		log[log::debug] << "Threads spawned" <<"\n";
		BasicIOThread::run();
	}
	catch (Exception &e) {
		log[log::error] << "Caught exception when initializing appbuilder ("
				<< e.what() << ")" <<"\n";
	}
	try {
		if (!stop_threads()) log[log::warning] << "Failed to stop threads!" <<"\n";
	}
	catch (Exception &e) {
		log[log::error] << "Caught exception while stopping threads ("
				<< e.what() << ")" <<"\n";
	}
	try {
		if (!delete_threads()) log[log::warning] << "Failed to delete threads!" <<"\n";
	}
	catch (Exception &e) {
		log[log::error] << "Caught exception while deleting threads ("
				<< e.what() << ")" <<"\n";
	}
	try {
		if (!delete_pipes()) log[log::warning] << "Failed to delete pipes!" <<"\n";
	}
	catch (Exception &e) {
		log[log::error] << "Caught exception while deleting pipes ("
				<< e.what() << ")" <<"\n";
	}
}
bool ApplicationBuilder::find_modules()
{
//	log[log::info] << "Looking for modules ("<<module_dirs.size() <<")!\n";
	BOOST_FOREACH(std::string p, module_dirs) {
		boost::filesystem::path p_(p);
		if (boost::filesystem::exists(p_) &&
				boost::filesystem::is_directory(p_)) {
			log[log::info] << "Looking for modules in " << p << "\n";
			for (boost::filesystem::directory_iterator dit(p_);
					dit !=boost::filesystem::directory_iterator(); ++dit) {
				boost::filesystem::directory_entry d = *dit;
				const std::string name = d.path().filename().string();
				if (name.substr(0,13)=="yuri2_module_") {
					log[log::debug] << "\tFound module " << name << "\n";
					modules.push_back(d.path().string());
				}
			}
		}
	}
	return true;
}
bool ApplicationBuilder::load_modules()
{
	BOOST_FOREACH(std::string s, modules) {
		try{
			bool loaded = RegisteredClass::load_module(s);
			//log[log::debug] << "Loading " << s<< ": "<<(loaded?"OK":"Failed") << "\n";
			if (loaded) log[log::info] << "Module " << s << " loaded successfully";
		}
		catch (yuri::exception::Exception &) {}

	}
	return true;
}
bool ApplicationBuilder::load_file(std::string path)
{
	//if (doc) delete doc;
	clear_tree();
	document_loaded = false;
	log[log::info] << "Loading " << path <<"\n";
	if (! doc.LoadFile(path)) {
		log[log::error] <<  "Failed to load file! " << doc.ErrorDesc()<<"\n";
		return false;
	}
	TiXmlElement * root = doc.RootElement();
	if (!root) {
		return false;
	}
	if (root->QueryValueAttribute("name",&appname)!=TIXML_SUCCESS) {
		appname="yuri";
	}


	TiXmlElement* node=0;
	node = root->FirstChildElement("general");
	if (node) {
		parse_parameters(*node,params);
	}
	init_local_params();

	node=0;
	while ((node = dynamic_cast<TiXmlElement*>(root->IterateChildren("module_dir",node)))) {
		if (!process_module_dir(*node)) continue;
	}
	find_modules();
	node=0;
	while ((node = dynamic_cast<TiXmlElement*>(root->IterateChildren("module",node)))) {
		if (!process_module(*node)) continue;
	}

	load_modules();

	node=0;
	while ((node = dynamic_cast<TiXmlElement*>(root->IterateChildren("variable",node)))) {
		if (!process_variable(*node)) continue;
	}

//	if (!check_variables()) return false;

	node=0;
	while ((node = dynamic_cast<TiXmlElement*>(root->IterateChildren("node",node)))) {
		if (!process_node(*node)) continue;
	}
	node = 0;
	while ((node = dynamic_cast<TiXmlElement*>(root->IterateChildren("link",node)))) {
		if (!process_link(*node)) continue;
	}
	node=0;
	node = root->FirstChildElement("description");
	if (node) {
		description=node->GetText();
	}
	return (document_loaded = true);
}
bool ApplicationBuilder::process_module_dir(TiXmlElement &node)
{
std::string path;
	if (node.QueryValueAttribute("path",&path)!=TIXML_SUCCESS) {
		log[log::error] << "Failed to load path attribute!" <<"\n";
		return false;
	}
	log[log::debug] << "Found module dir" << path << "\n";
	module_dirs.push_back(path);
	return true;
}
bool ApplicationBuilder::process_module(TiXmlElement &node)
{
std::string path;
	if (node.QueryValueAttribute("path",&path)!=TIXML_SUCCESS) {
		log[log::error] << "Failed to load path attribute!" <<"\n";
		return false;
	}
	log[log::debug] << "Found module " << path << "\n";
	modules.push_back(path);
	return true;
}


bool ApplicationBuilder::process_node(TiXmlElement &node)
{
std::string name,cl;
	if (node.QueryValueAttribute("name",&name)!=TIXML_SUCCESS) {
		log[log::error] << "Failed to load name attribute!" <<"\n";
		return false;
	}
	if (node.QueryValueAttribute("class",&cl)!=TIXML_SUCCESS) {
		log[log::error] << "Failed to load class attribute!" <<"\n";
		return false;;
	}
	log[log::debug] << "Found Node " << name << " of class " << cl <<"\n";
	if (nodes.find(name) != nodes.end()) {
		log[log::warning] << "Multiple nodes with name " << name << \
				" specified! (was " << nodes[name]->type << ", found"
				<< cl << "). Discarding the the later" <<"\n";
		return false;
	}

	if (!RegisteredClass::is_registered(cl)) {
		log[log::warning] << "Requested node of class " << cl
				<< " that is not registered! Discarding" <<"\n";
		return false;
	}
	shared_ptr<NodeRecord> n(new NodeRecord(name,cl));

	if (!parse_parameters(node,n->params)) {
		log[log::warning] << "Failed to parse parameters. Discarding" <<"\n";
		//delete n;
		return false;
	}
	std::pair<std::string,shared_ptr<Parameter> > parameter;
	BOOST_FOREACH(parameter,n->params.params) {
	std::string value = parameter.second->get<std::string>();
		if (value[0] == '@') {
		std::string argname = value.substr(1);
			n->variables[parameter.first]=argname;
		}
	}

	log[log::debug] << "Storing node " << name <<"\n";
	nodes[name]=n;
	return true;
}

bool ApplicationBuilder::process_link(TiXmlElement &node)
{
	std::string name,src,target;
	if (node.QueryValueAttribute("name",&name)!=TIXML_SUCCESS) {
		log[log::error] << "Failed to load name attribute!" <<"\n";
		return false;
	}
	if (node.QueryValueAttribute("source",&src)!=TIXML_SUCCESS) {
		log[log::error] << "Failed to load source attribute for link "
				<< name << "!" <<"\n";
		return false;
	}
	if (node.QueryValueAttribute("target",&target)!=TIXML_SUCCESS) {
		log[log::error] << "Failed to load target attribute for link "
				<< name << "!" <<"\n";
		return false;
	}
	int src_ipos = src.find_last_of(':');
	int target_ipos = target.find_last_of(':');
	std::string srcnode = src.substr(0,src_ipos);
	std::string targetnode = target.substr(0,target_ipos);
	int srci = boost::lexical_cast<int>(src.substr(src_ipos+1));
	int targeti = boost::lexical_cast<int>(target.substr(target_ipos+1));
	bool remote_link = false;
	if (srcnode == "$$" || targetnode=="$$") remote_link = true;
	if (!remote_link) {
		log[log::debug] << "link from " <<  srcnode << "[" << srci << "]" <<
				"to " << targetnode << "[" <<targeti << "]" <<"\n";
		if (nodes.find(srcnode)==nodes.end()) {
			log[log::warning] << "Source node for the link (" << srcnode
					<< ") not specified! Discarding link." <<"\n";
			return false;
		}
		if (nodes.find(targetnode)==nodes.end()) {
			log[log::warning] << "Target node for the link (" << targetnode
					<< ") not specified! Discarding link." <<"\n";
			return false;
		}
	}
	shared_ptr<LinkRecord> l(new LinkRecord(name,srcnode,targetnode,srci,targeti));
	if (!parse_parameters(node,l->params)) {
		log[log::warning] << "Failed to parse parameters. Discarding" <<"\n";
		//delete l;
		return false;
	}
	std::pair<std::string,shared_ptr<Parameter> > parameter;
	BOOST_FOREACH(parameter,l->params.params) {
	std::string value = parameter.second->get<std::string>();
		if (value[0] == '@') {
		std::string name = value.substr(1);
			l->variables[parameter.first]=name;
		}
	}
	log[log::debug] << "Storing link " << name <<"\n";
	if (!remote_link) {
		if (links.count(name)) {
			log[log::warning] << "Replacing link '" << name <<"'. This is probably an error ;)";
		}
		links[name]=l;
	} else {
		if (srcnode == "$$") {
			input_pipes[srci]=std::make_pair(l,pBasicPipe());
			resize(std::max<size_t>(in_ports,srci+1),out_ports);
		} else {
			output_pipes[targeti]=std::make_pair(l,pBasicPipe());
			resize(srci,std::max<size_t>(out_ports,targeti+1));
		}
	}
	return true;
}

bool ApplicationBuilder::process_variable(TiXmlElement &node)
{
	std::string name,def_value;
	bool required;
	if (node.QueryValueAttribute("name",&name)!=TIXML_SUCCESS) {
		log[log::error] << "Failed to load name attribute!" <<"\n";
		return false;
	}
	if (node.QueryValueAttribute("default",&def_value)!=TIXML_SUCCESS) {
		log[log::error] << "Failed to load default value for variable "
				<< name << "!" <<"\n";
		return false;
	}

	if (node.QueryValueAttribute("required",&required)!=TIXML_SUCCESS) {
		required = false;
	}
	std::string desc;
	const char *d = node.GetText();
	if (d) desc = d;
	shared_ptr<VariableRecord> var(new VariableRecord(name,def_value,required,desc));
	log[log::debug] << "Storing variable " << name <<"\n";
	variables[name]=var;
	return true;
}


bool ApplicationBuilder::parse_parameters(TiXmlElement &element,Parameters &params)
{
	TiXmlElement *node=0;
	while ((node = dynamic_cast<TiXmlElement*>(element.IterateChildren("parameter",node)))) {
	std::string name, value;
		if (node->QueryValueAttribute("name",&name)!=TIXML_SUCCESS) {
			log[log::error] << "Failed to load name attribute!" <<"\n";
			return false;
		}
		if (node->FirstChild("parameter")) {

			Parameters& par_child = params[name].push_group();
			log[log::info] << "Found group parameter " << name << ", added "
					<< params[name].parameters_vector.size() << ". value" <<"\n";
			if (!parse_parameters(*node,par_child)) return false;
		} else {
			value = node->GetText();
			log[log::debug] << "Found parameter " << name << " with value " << value <<"\n";
			params[name]=value;
		}
	}
	return true;
}

void ApplicationBuilder::clear_tree()
{
	nodes.clear();
	links.clear();
}

bool ApplicationBuilder::prepare_threads()
{
	if (threads_prepared) return true;
	std::pair<std::string,shared_ptr<NodeRecord> > node;
	BOOST_FOREACH(node,nodes) {
		log[log::debug] << "Preparing node " << node.first <<"\n";
		assert(node.second.get());
		assert(RegisteredClass::is_registered(node.second->type));
		shared_ptr<Instance> instance = RegisteredClass::prepare_instance(node.second->type);
		assert(instance.get());
		(*instance->params)["node_name"]=node.first;
		instance->params->merge(node.second->params);
		pParameters vars = assign_variables(node.second->variables);
		instance->params->merge(*vars);
		log[log::debug] << "Node " << node.first << " of class " << node.second->type
				<< " will be started with parameters:" <<"\n";
		show_params(*instance->params);
		try {
			threads[node.first] = instance->create_class(log,get_this_ptr());
		}
		catch (exception::InitializationFailed &e) {
			log[log::error] << "Failed to create node '" << node.first << "' (" << node.second->type<<"): " << e.what() <<"\n";
			return false;
		}
		catch (Exception &e) {
			throw Exception(std::string("[")+node.first+std::string("] ")+std::string(e.what()));
		}
	}
	threads_prepared = true;
	return true;
}

void ApplicationBuilder::show_params(Parameters& _params, std::string prefix)
{
	std::pair<std::string,shared_ptr<Parameter> > par;
	BOOST_FOREACH(par,(_params.params)) {
		if (par.second->type==GroupType) {
			for (int i=0;;++i) {
				try {
					Parameters &p = (*par.second)[i];
					log[log::info] << prefix << par.first << "["<<i<<"] is group with following values:" <<"\n";
					show_params(p,prefix+'\t');
				}
				catch (exception::OutOfRange &e) {
					break;
				}
			}
		} else {
			log[log::debug] << prefix << par.first << " has value " << par.second->get<std::string>() <<"\n";
		}
	}
}

bool ApplicationBuilder::prepare_links()
{
	std::pair<std::string,shared_ptr<LinkRecord> > link;
	BOOST_FOREACH(link,links) {
		log[log::debug] << "Preparing link " << link.first <<"\n";
		assert(link.second.get());
		pParameters params (new Parameters(*default_pipe_param));
		params->merge(link.second->params);
		pParameters vars = assign_variables(link.second->variables);
		params->merge(*vars);
		shared_ptr<BasicPipe> p = BasicPipe::generator(log,link.first,*params);
		threads[link.second->source_node]->connect_out(link.second->source_index,p);
		threads[link.second->target_node]->connect_in(link.second->target_index,p);
		pipes[link.first]=p;
	}
	std::map<size_t, std::pair<shared_ptr<LinkRecord>, pBasicPipe > >::iterator it = input_pipes.begin();
	for (;it != input_pipes.end(); ++it) {
		std::pair<shared_ptr<LinkRecord>, pBasicPipe >& l = it->second;
		log[log::info] << "Connecting input pipe " <<  it->first << " " << l.first->name << " to " << l.first->target_node;
		pBasicIOThread node = get_node(l.first->target_node);
		node->connect_in(l.first->target_index, l.second);
		l.second.reset();
	}
	for (it=output_pipes.begin();it != output_pipes.end(); ++it) {
		std::pair<shared_ptr<LinkRecord>, pBasicPipe >& l = it->second;
		log[log::info] << "Connecting output pipe " <<  it->first << " " << l.first->name << " to " << l.first->source_node;
		pBasicIOThread node = get_node(l.first->source_node);
		node->connect_in(l.first->source_index, l.second);
		l.second.reset();
	}
	return true;
}
bool ApplicationBuilder::spawn_threads()
{
	std::pair<std::string,pBasicIOThread > thread;
	BOOST_FOREACH(thread,threads) {
		spawn_thread(thread.second);
		//log[log::info] << "Thread for object " << thread.first << " spawned as " << lexical_caststd::string>(children[thread.second]->thread_ptr->native_handle()) << ", boost id: "<< children[thread.second]->thread_ptr->get_id()<<"\n";
		tids[thread.first]=0;
	}
	return true;
}

bool ApplicationBuilder::stop_threads()
{
	return true;
}
bool ApplicationBuilder::delete_threads()
{
	return true;
}
bool ApplicationBuilder::delete_pipes()
{
	return true;
}

bool ApplicationBuilder::step()
{
	if (!start_time.is_not_a_date_time()) {
		if (start_time + run_limit < boost::posix_time::microsec_clock::local_time()) {
			log[log::info] << "Time limit reached (" << to_simple_string(run_limit)
					<< "), returning. Started " << to_simple_string(start_time.time_of_day())
					<< ", actual time is " << boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time().time_of_day())
					<<"\n";
			return false;
		}
	}
	fetch_tids();
	return true;
}
pBasicIOThread ApplicationBuilder::get_node(std::string id)
{
	if (threads.find(id)==threads.end()) {
		std::pair<std::string,pBasicIOThread > p;
		BOOST_FOREACH(p,threads) {
			log[log::debug] << "I have " << p.first <<"\n";
		}
		throw Exception(std::string(id)+" does not exist");
	}
	return threads[id];
}

void ApplicationBuilder::init()
{
	log[log::info] << "Populating default module paths\n";
	module_dirs.push_back("./modules/");
	module_dirs.push_back("./bin/modules/");
	#ifdef INSTALL_PREFIX
		module_dirs.push_back(INSTALL_PREFIX "/lib/yuri2/");
	#else
		module_dirs.push_back("/usr/lib/yuri2/");
	#endif
	const std::string filename=params["config"].get<std::string>();
	if (filename!="") {
		if (!load_file(filename)) throw exception::InitializationFailed(std::string("Failed to load file ")+filename);
	}

}

void ApplicationBuilder::init_local_params()
{
	set_params(params);
}

void ApplicationBuilder::fetch_tids()
{
#ifdef __linux__
	std::pair<std::string,pid_t> tid_record;
	bool changed = false;
	BOOST_FOREACH(tid_record,tids) {
		if (!tid_record.second) {
			tids[tid_record.first] = threads[tid_record.first]->get_tid();
			log[log::debug] << "Thread " << tid_record.first << " has tid " << tids[tid_record.first] <<"\n";
			changed = true;
		}
	}
	if (changed) {
		std::string destdir =std::string("/tmp/")+lexical_cast<std::string>(getpid());
		boost::filesystem::path p(destdir);
		if (!boost::filesystem::exists(p)) {
			boost::filesystem::create_directory(p);
		}
		BOOST_FOREACH(tid_record,tids) {
			if (tid_record.second) {
				boost::filesystem::path p2 = p / lexical_cast<std::string>(tid_record.second); //(destdir+lexical_cast<std::string>(tid_record.second));
				if (!boost::filesystem::exists(p2)) {
					std::ofstream of(p2.string().c_str());
					of << tid_record.first << "\n";
				}
			}
		}
	}
#endif
}

pParameters ApplicationBuilder::assign_variables(std::map<std::string, std::string> vars)
{
	std::pair<std::string, std::string> var;
	pParameters var_params(new Parameters());
	BOOST_FOREACH(var,vars) {
		if (variables.find(var.second) != variables.end()) {
			(*var_params)[var.first] = variables[var.second]->value;
		}
	}
	return var_params;
}

void ApplicationBuilder::parse_argv(std::vector<std::string> argv)
{
	log[log::debug] << "" << argv.size() << " arguments to parse" <<"\n";
std::string arg;
	BOOST_FOREACH(arg,argv) {
		size_t delim = arg.find('=');
		if (delim==std::string::npos) {
			log[log::warning] << "Failed to parse argument '" <<arg<<"'" <<"\n";
		} else {
		std::string name, value;
			name = arg.substr(0,delim);
			value = arg.substr(delim+1);
			log[log::debug] << "Parsed argument " << name << " -> " << value <<"\n";
			if (variables.find(name) != variables.end()) {
				variables[name]->value=value;
			} else {
				variables[name]=shared_ptr<VariableRecord>(new VariableRecord(name,value));
			}
		}

	}
}
bool ApplicationBuilder::set_param(const core::Parameter& parameter)
{
	using boost::iequals;
	if (iequals(parameter.name, "show_time")) {
		bool val = parameter.get<bool>();
		log.set_flags((log.get_flags()&~log::show_time)|(val?log::show_time:0));
	} else if (iequals(parameter.name, "show_level")) {
		bool val = parameter.get<bool>();
		log.set_flags((log.get_flags()&~log::show_level)|(val?log::show_level:0));
	} else if (iequals(parameter.name, "use_colors")) {
		bool val = parameter.get<bool>();
		log.set_flags((log.get_flags()&~log::use_colors)|(val?log::use_colors:0));
	} else if (iequals(parameter.name, "skip_verification")) {
		skip_verification = parameter.get<bool>();
	} else return core::BasicIOThread::set_param(parameter);
	return true;
}
bool ApplicationBuilder::check_variables()
{
	for (std::map<std::string, shared_ptr<VariableRecord> >::iterator it = variables.begin();
			it != variables.end(); ++it) {
		shared_ptr<VariableRecord>& v = it->second;
		if (params.is_defined(v->name)) {
			v->value = params[v->name].get<std::string>();
		}
		if (v->required && v->value.empty()) {
			throw InitializationFailed(std::string("No value specified for required variable '")+v->name+std::string("'"));
		}
	}
	return true;
}
std::string ApplicationBuilder::get_appname()
{
	return appname;
}
std::string ApplicationBuilder::get_description()
{
	return description;
}
const std::map<std::string,shared_ptr<VariableRecord> >& ApplicationBuilder::get_variables()
{
	return variables;
}
void ApplicationBuilder::connect_in(yuri::sint_t index, pBasicPipe pipe)
{
	if (!input_pipes.count(index)) {
		log[log::error] << "Attempt to connect input pipe to unpopulated index " << index;
		return;
	}
	std::pair<shared_ptr<LinkRecord>, pBasicPipe>& link = input_pipes[index];
	try {
		log[log::info] << "Connecting input pipe " << link.first->name << " targeted to " << link.first->target_node;
		link.second = pipe;
		pipe->get_notification_fd();
	}
	catch (Exception& ) {
//		log[log::error] << "Node ("<< link.second->target_node <<") specified for input pipe " << index << " was not created...";
		return;
	}

}
void ApplicationBuilder::connect_out(yuri::sint_t index, pBasicPipe pipe)
{
	if (!output_pipes.count(index)) {
		log[log::error] << "Attempt to connect input pipe to unpopulated index " << index;
		return;
	}
	std::pair<shared_ptr<LinkRecord>, pBasicPipe>& link = input_pipes[index];
	try {
		log[log::info] << "Connecting input pipe " << link.first->name << " targeted to " << link.first->target_node;
		link.second = pipe;
		pipe->get_notification_fd();
	}
	catch (Exception& ) {
		return;
	}
}
}

}
