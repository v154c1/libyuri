/*!
 * @file 		test_module.cpp
 * @author 		Zdenek Travnicek
 * @date 		12.2.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include <iostream>
#include <cstdint>
#if defined __linux__ || defined(__FreeBSD__) || defined(__OpenBSD__) || defined __CYGWIN__
#include <dlfcn.h>
#define test_posix
#elif defined _WIN32
#include <windows.h>
#endif
#include <string>

bool test_file(const std::string& filename)
{
#if defined test_posix
	void *handle=dlopen(filename.c_str(),RTLD_NOW|RTLD_GLOBAL);
#elif defined _WIN32
	HINSTANCE handle = LoadLibrary(filename.c_str());
#endif
	if (!handle) {
		std::cerr << "Failed to open file " << filename
#if defined test_posix
			<<": " << dlerror() 
#endif
			<<"\n";
		return 1;
	}
	typedef const char * (*get_name_t)(void);
	typedef void (*register_module_t)(void);
#if defined test_posix
	// The ugly cast to uintptr_t is here for the sole purpose of silencing g++ warnings.
	get_name_t get_name = reinterpret_cast<get_name_t>(reinterpret_cast<uintptr_t>(dlsym(handle,"yuri2_8_module_get_name")));
	register_module_t register_module = reinterpret_cast<register_module_t>(reinterpret_cast<uintptr_t>(dlsym(handle,"yuri2_8_module_register")));
#elif defined _WIN32
	get_name_t get_name = reinterpret_cast<get_name_t>(GetProcAddress(handle,"yuri2_8_module_get_name"));
	register_module_t register_module = reinterpret_cast<register_module_t>(GetProcAddress(handle,"yuri2_8_module_register"));
#endif
	bool valid = true;
	if (!get_name) {
		std::cerr << "Module doesn't export yuri_module_get_name\n";
		valid = false;
	}
	if (!register_module) {
		std::cerr << "Module doesn't export yuri_module_register\n";
		valid = false;
	}
	if (valid) {
		const char* name = get_name();
		if (!name) {
			std::cerr << "Module doesn't return it's name\n";
			valid = false;
		} else {
			std::cerr << "Module " << name << " seem valid\n";
		}
	}
#if defined test_posix
	dlclose(handle);
#elif defined _WIN32
	FreeLibrary(handle);
#endif
	if (!valid) {
		return 1;
	}
	return 0;
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cerr << "Usage " << argv[0] <<" <path-to-module> <path-to-module> ...\n";
		return 1;
	}
	int res = 0;
	for (int i=1;i<argc;++i) {
		res |= test_file(argv[i]);
	}
	return res;
}


