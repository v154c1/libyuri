/*!
 * @file 		ModuleLoader.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		15.9.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "ModuleLoader.h"
#include "yuri/core/utils/platform.h"
#include "yuri/core/utils/make_unique.h"
#include "yuri/core/utils/DirectoryBrowser.h"
#include "yuri/core/utils.h"
#include <stdexcept>
#include <iostream>


#if defined YURI_POSIX
#include <dlfcn.h>
#elif defined YURI_WIN
#include <windows.h>
#else
#error Runtime object loading not supported on this platform
#endif

namespace yuri {
namespace core {
namespace module_loader {

const std::string module_prefix = "yuri2.8_module_";
const std::string module_get_name = "yuri2_8_module_get_name";
const std::string module_register = "yuri2_8_module_register";

struct dynamic_loader::dynamic_loader_pimpl_ {
#if defined YURI_POSIX
	void* handle;
#elif defined YURI_WIN
	HINSTANCE handle;
#endif
};




dynamic_loader::dynamic_loader(dynamic_loader&& rhs) noexcept
:pimpl_(std::move(rhs.pimpl_))
{
rhs.reset();
}


dynamic_loader& dynamic_loader::operator=(dynamic_loader&& rhs) noexcept
{
	pimpl_=std::move(rhs.pimpl_);
	return *this;
}

void dynamic_loader::reset()
{
	pimpl_.reset();
}


#if defined YURI_POSIX
dynamic_loader::dynamic_loader(const std::string& path)
:pimpl_(make_unique<dynamic_loader::dynamic_loader_pimpl_>())
{
	pimpl_->handle = dlopen(path.c_str(),RTLD_LAZY);
	if (!pimpl_->handle) throw std::runtime_error("Failed to open handle "+path + " ("+dlerror()+")");
}
dynamic_loader::~dynamic_loader() noexcept
{
	delete_handle();
}
void dynamic_loader::delete_handle()
{
	if (pimpl_ && pimpl_->handle) {
		dlclose(pimpl_->handle);
	}
	reset();
}
void* dynamic_loader::load_symbol_impl(const std::string& symbol)
{
	return dlsym(pimpl_->handle,symbol.c_str());
}
#elif defined YURI_WIN
dynamic_loader::dynamic_loader(const std::string& path)
:pimpl_(make_unique<dynamic_loader::dynamic_loader_pimpl_>())
{
	pimpl_->handle = LoadLibrary(path.c_str());
	if (!pimpl_->handle) throw std::runtime_error("Failed to open handle");
}
dynamic_loader::~dynamic_loader()
{
	delete_handle();
}
void dynamic_loader::delete_handle()
{
	if (pimpl_ && pimpl_->handle) FreeLibrary(pimpl_->handle);
	reset();
}
void* dynamic_loader::load_symbol(const std::string& symbol)
{
	return reinterpret_cast<void*>(GetProcAddress(handle,symbol.c_str()));
}

#endif
namespace {
std::vector<std::string> built_in_paths = {
		"./modules",
		"./bin/modules",
		"../modules",
#ifdef INSTALL_PREFIX
		INSTALL_PREFIX "/lib/yuri2/",
#else
		"/usr/lib/yuri2",
#endif
};
}
const std::vector<std::string>& get_builtin_paths()
{
	return built_in_paths;
}

std::vector<std::string> find_modules_path(const std::string& path)
{
	std::vector<std::string> modules;
	const auto& files = filesystem::browse_files(path, module_prefix);
	for (const auto& pp: files) {
//		std::cout << "x: " << pp << "\n";
		modules.push_back(pp);
	}
	return modules;
}

using get_name_t 		= const char * (*)(void);
using register_module_t = int (*)(void);

namespace {
	bool leak_current_handle = false;
}

bool load_module(const std::string& path)
{
	static std::mutex loader_mutex;
	lock_t _(loader_mutex);
	static std::map<std::string, dynamic_loader> loaded_files;
	if (contains(loaded_files, path)) return true;
	try {
		get_name_t get_name = nullptr;
		register_module_t register_module = nullptr;

		dynamic_loader loader (path);

		get_name = loader.load_symbol<get_name_t>(module_get_name);
		register_module = loader.load_symbol<register_module_t>(module_register);
		leak_current_handle = false;
		if (get_name && register_module && register_module() == 0) {
			if (leak_current_handle) {
				loader.reset();
			}
			loaded_files.insert(std::make_pair(path,std::move(loader)));
			return true;
		}
	}
	catch (std::runtime_error&) {}
	return false;
}

void leak_module_handle()
{
	leak_current_handle = true;
}

}
}
}




