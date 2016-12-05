/*!
 * @file 		frei0r_register.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		05.06.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "Frei0rWrapper.h"
#include "Frei0rSource.h"
#include "Frei0rMixer.h"
#include "yuri/core/Module.h"

#include "yuri/core/utils/DirectoryBrowser.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/utils/environment.h"
#include <algorithm>
namespace yuri {
namespace frei0r {

namespace {

template<class T>
std::function<decltype(T::configure())()> generate_configure(const std::string& name, const frei0r_module_t& module)
{
	auto description = std::string("[Frei0r] ") + module.info.name+" " +
			std::to_string(module.info.major_version)+"."+std::to_string(module.info.minor_version) +
			" (" + module.info.author+")";
	if (module.info.explanation) {
		description+=std::string(" - ")+module.info.explanation;
	}
	auto p = T::configure();
	p.set_description(description);
	p["_frei0r_path"]=name;
	auto instance = module.construct(8,8);
	if (module.get_param_info) {
		f0r_param_info_t param;
		for (auto i: irange(module.info.num_params)) {
			module.get_param_info(&param, i);
			if (!param.name) continue;
			std::string desc = param.explanation?param.explanation:"";
			if (module.get_param_value) {
				switch(param.type) {
					case F0R_PARAM_DOUBLE: {
						f0r_param_double fp;
						module.get_param_value(instance, &fp, i);
						p[param.name][desc] = fp;
					} break;
					case F0R_PARAM_BOOL: {
						f0r_param_bool fp;
						module.get_param_value(instance, &fp, i);
						p[param.name][desc] = fp > 0.5;
					} break;
					case F0R_PARAM_POSITION: {
						f0r_param_position fp;
						module.get_param_value(instance, &fp, i);
						p[std::string(param.name)+"_x"][desc] = fp.x;
						p[std::string(param.name)+"_y"][desc] = fp.y;
					} break;
					case F0R_PARAM_COLOR: {
						f0r_param_color fp;
						module.get_param_value(instance, &fp, i);
						p[std::string(param.name)+"_r"][desc] = fp.r;
						p[std::string(param.name)+"_g"][desc] = fp.g;
						p[std::string(param.name)+"_b"][desc] = fp.b;
					} break;
					case F0R_PARAM_STRING:
						f0r_param_string fp;
						module.get_param_value(instance, &fp, i);
						p[param.name][desc] = fp;
						break;
					default:
						p[param.name][desc]="UNKNOWN";
						break;
				}
			}
		}
	}
	module.destruct(instance);
	return [p]()->IOThreadGenerator::param_type {
		return p;
	};
}
std::vector<std::string> add_vendor_subdirectories(std::vector<std::string> dirs)
{
	std::vector<std::string> dir2;
	for(auto&d: dirs) {
		auto&& sdirs = core::filesystem::browse_directories(d);
		dir2.push_back(std::move(d));
		dir2.insert(dir2.end(), sdirs.begin(), sdirs.end());
	}
	return dir2;
}
std::vector<std::string> get_plugin_directories()
{
	auto dirs = core::utils::get_environment_path("FREI0R_PATH");
	if (dirs.empty()) {
#ifdef YURI_POSIX
		auto home = core::utils::get_environment_variable("HOME");
		if (!home.empty()) {
			dirs.push_back(home+"/.frei0r-1/lib/");
		}
		dirs.push_back("/usr/lib/frei0r-1/");
		dirs.push_back("/usr/local/lib/frei0r-1/");
#endif
	}
	return add_vendor_subdirectories(std::move(dirs));
}

std::vector<std::string> list_plugins()
{
	std::vector<std::string> files;
	for (const auto& dir: get_plugin_directories()) {
		auto&& f = core::filesystem::browse_files(dir, "", ".so");
		files.reserve(files.size() + f.size());
		files.insert(files.end(), f.begin(), f.end());
	}

	std::sort(files.begin(), files.end());
	files.erase(std::unique(files.begin(), files.end()), files.end());

	return files;
}


}


MODULE_REGISTRATION_BEGIN("frei0r")
		const auto files = list_plugins();
		for (const auto& f: files) {
			try {
				frei0r_module_t module(f);
				switch (module.info.plugin_type) {
					case F0R_PLUGIN_TYPE_FILTER: {
						REGISTER_IOTHREAD_FUNC("frei0r_"+core::filesystem::get_filename(f, false),
							Frei0rWrapper::generate,
							generate_configure<Frei0rWrapper>(f, module))
					} break;
					case F0R_PLUGIN_TYPE_SOURCE: {
						REGISTER_IOTHREAD_FUNC("frei0r_source_"+core::filesystem::get_filename(f, false),
							Frei0rSource::generate,
							generate_configure<Frei0rSource>(f, module))
					} break;
					case F0R_PLUGIN_TYPE_MIXER2:
					case F0R_PLUGIN_TYPE_MIXER3:{
						REGISTER_IOTHREAD_FUNC("frei0r_mixer_"+core::filesystem::get_filename(f, false),
							Frei0rMixer::generate,
							generate_configure<Frei0rMixer>(f, module))
					} break;
				}
			}
			catch (std::exception& e)
			{
				std::cout << "Failed to load " << f << " ("<<e.what() <<")"<<std::endl;
			}

		}
		//REGISTER_IOTHREAD("frei0r",Frei0rWrapper)
		//REGISTER_IOTHREAD("frei0r_source",Frei0rSource)

		// This is ugly... but it is necesary now
		core::module_loader::leak_module_handle();
MODULE_REGISTRATION_END()


}
}


