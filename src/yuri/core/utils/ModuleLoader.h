/*!
 * @file 		ModuleLoader.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		15.9.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef MODULELOADER_H_
#define MODULELOADER_H_
#include <string>
#include <vector>
#include <type_traits>
#include "yuri/core/utils/new_types.h"
namespace yuri {
namespace core {
namespace module_loader {

struct dynamic_loader {
	dynamic_loader(const std::string& path);
	~dynamic_loader() noexcept;
	dynamic_loader(const dynamic_loader&) = delete;
	dynamic_loader(dynamic_loader&& rhs) noexcept;
	dynamic_loader& operator=(const dynamic_loader&) = delete;
	dynamic_loader& operator=(dynamic_loader&& rhs) noexcept;
	void delete_handle();
	void reset();

	template<typename T>
	typename std::enable_if<std::is_pointer<T>::value,T>::type
	load_symbol(const std::string& symbol) {
		return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(load_symbol_impl(symbol)));
	}
private:
	struct dynamic_loader_pimpl_;
	void *load_symbol_impl(const std::string& symbol);

	std::unique_ptr<dynamic_loader_pimpl_> pimpl_;
};


EXPORT std::vector<std::string> find_modules_path(const std::string& path);
EXPORT bool load_module(const std::string& path);
EXPORT const std::vector<std::string>& get_builtin_paths();

/*!
 * This method LEAKS the handle, effectively preventing libyuri unloading it at the end.
 * it's STRONGLY RECOMMENDED NOT TO USE this method, unless really needed.
 * It has to be called from register_module() function during loading.
 * Currently it's needed for frei0r module.
 */
EXPORT void leak_module_handle();
}
}
}



#endif /* MODULELOADER_H_ */
