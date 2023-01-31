/*!
 * @file 		InputThread.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		13. 3. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "InputThread.h"

namespace yuri {
namespace core{
namespace detail {

bool is_last_key(const std::vector<core::InputDeviceConfig>& cfgs, const std::string& key, const std::vector<std::string>& used)
{
	for (const auto& c: cfgs) {
		for (const auto& p: c.params) {
			if (p.first != key && !contains(used, p.first)) {
				return false;
			}
		}
	}
	return true;
}

std::vector<std::string> unused_keys(const core::InputDeviceConfig& cfg, const std::vector<std::string>& used)
{
	std::vector<std::string> keys;
	for (const auto& p: cfg.params) {
		if (!contains(used, p.first)) {
			keys.push_back(p.first);
		}
	}
	return keys;
}

void print_cfgs_k(log::Log& log, log::debug_flags flags, const std::vector<core::InputDeviceConfig>& cfgs, std::vector<std::string> order, std::vector<std::string> used, std::string prefix)
{
//	log[log::info] << "pcfg_k";
	if (cfgs.empty()) return;
	if (order.empty()) {
		// We used up all keys and there's still some params, so lets print them here
		for (const auto& cfg: cfgs) {
			auto keys = unused_keys(cfg, used);
			// This move is necessary only for GCC4.7 which doesn't support ref-qualified member functions
//			auto l = std::move(log[flags] << prefix);
            auto l = log[flags];
            l << prefix;
			for (const auto& key: keys) {
				l << "key: " << cfg.params[key].get<std::string>() << ", ";
			}
		}
		return;
	}
	const auto& key = order[0];
	auto used_next = used;
	used_next.push_back(key);
	if (is_last_key(cfgs, key, used)) {
		// We have last key, so let's use short output
		//auto l = std::move(log[flags] << prefix << key << ": ");
        auto l = log[flags];
        l << prefix << key << ": ";
		core::utils::print_list(l, find_map_key_values<std::string>(cfgs, key));
	} else {
		for (const auto& c: find_submap_by_key<std::string>(cfgs, key)) {
			log[flags] << prefix << key << ": " << c.first;
			print_cfgs_k(log, flags, c.second, std::vector<std::string>(order.begin()+1, order.end()), used_next, prefix+"\t");
		}
	}
}

}

void print_cfgs(log::Log& log, log::debug_flags flags, const core::InputDeviceInfo& info)
{
	detail::print_cfgs_k(log, flags, info.configurations, info.main_param_order, std::vector<std::string>{}, "\t");
}

}
}





