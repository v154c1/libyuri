/*!
 * @file 		json_helpers.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		20. 10. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef CXX_SRC_JSON_HELPERS_H_
#define CXX_SRC_JSON_HELPERS_H_

#include "json/json.h"

namespace yuri {
namespace linky {

inline const Json::Value& get_nested_element(const Json::Value& root) {
  return root;
}

template <class... Ts>
const Json::Value& get_nested_element(const Json::Value& root,
                                      const std::string& idx, Ts&&... idxs);

template <class... Ts>
const Json::Value& get_nested_element(const Json::Value& root,
                                      Json::ArrayIndex idx, Ts&&... idxs) {
  if (root.isArray() && root.size() > idx) {
    return get_nested_element(root[idx], std::forward<Ts>(idxs)...);
  }
  throw std::runtime_error("Bad index ("+std::to_string(idx)+")");
}

template <class... Ts>
const Json::Value& get_nested_element(const Json::Value& root,
                                      const std::string& idx, Ts&&... idxs) {
  if (root.isMember(idx)) {
    return get_nested_element(root[idx], std::forward<Ts>(idxs)...);
  }
  throw std::runtime_error("Bad index ("+idx+")");
}


template<class T>
typename std::enable_if<std::is_same<typename std::decay<T>::type, bool>::value, bool>::type
get_json_value_as (const Json::Value& v)
{
	if (v.type() != Json::booleanValue) {
		throw std::runtime_error("Bad value");
	}
	return v.asBool();
}

template<class T>
typename std::enable_if<std::is_same<typename std::decay<T>::type, std::string>::value, std::string>::type
get_json_value_as(const Json::Value& v)
{
	return v.asString();
}

template<class T>
typename std::enable_if<std::is_same<typename std::decay<T>::type, char const *>::value, std::string>::type
get_json_value_as(const Json::Value& v)
{
	return v.asString();
}

template<class T>
typename std::enable_if<std::is_integral<T>::value && !std::is_same<typename std::decay<T>::type, bool>::value, typename std::decay<T>::type>::type
get_json_value_as(const Json::Value& v)
{
	if (v.type() != Json::intValue && v.type() != Json::uintValue && v.type() != Json::realValue) {
		throw std::runtime_error("Bad value");
	}
	return static_cast<T>(v.asInt());
}

template<class T>
typename std::enable_if<std::is_floating_point<T>::value, typename std::decay<T>::type>::type
get_json_value_as(const Json::Value& v)
{
	if (v.type() != Json::realValue && v.type() != Json::intValue && v.type() != Json::uintValue){
		throw std::runtime_error("Bad value");
	}
	return static_cast<T>(v.asDouble());
}


template<class T, class... Ts>
decltype(get_json_value_as<typename std::decay<T>::type>(Json::Value()))
get_nested_value_or_default(const Json::Value& root, T&& def, Ts... idxs)
{
	using TT = typename std::decay<T>::type;
	try {
		const auto& v = get_nested_element(root, idxs...);
		return get_json_value_as<TT>(v);
	}
	catch(...) {
		return std::forward<T>(def);
	}
}

template<class T, class... Ts>
std::vector<T> get_vector_from_element(const Json::Value& cfg, Ts... args)
{
	std::vector<T> ret;
	try {
		const auto& e = get_nested_element(cfg, args...);
		if (!e.isArray()) return ret;
		for (const auto& v: e) {
			ret.push_back(get_json_value_as<T>(v));
		}
	}
	catch(std::exception&) {

	}
	return ret;
}

}
}
#endif /* CXX_SRC_JSON_HELPERS_H_ */
