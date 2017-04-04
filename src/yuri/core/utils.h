/*!
 * @file 		utils.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		9.6.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef UTILS_H_
#define UTILS_H_
#include <map>
#include <vector>
#include <sstream>
#include <cctype>
#include <mutex>
#include <iterator>
#include <algorithm>
namespace yuri {

/*!
 * @brief Ancillary class for initializing std::map
 *
 * These clases were made obsolete by C++11 and initializer lists, both will be removed in near future.
 *
 * Usage std::map<A, B> name_of_map = map_list_of<A, B>(a0, b0)(a1, b1)(a2, b2);
 */
template<class Key, class Value> class map_list_of {
public:
	operator std::map<Key, Value>() { return tmp_map; }
	map_list_of() {}
	map_list_of(const Key& key, const Value& value) {tmp_map[key]=value;}
	map_list_of & operator() (const Key& key, const Value& value) {tmp_map[key]=value;return *this;}
protected:
	std::map<Key, Value> tmp_map;
};

template<class Value> class list_of {
public:
	operator std::vector<Value>() { return tmp_vec; }
	list_of() {}
	list_of(const Value& val) { tmp_vec.push_back(val); }
	list_of & operator() (const Value& val) {tmp_vec.push_back(val); return *this;}
protected:
	std::vector<Value> tmp_vec;
};

struct bad_lexical_cast: public std::runtime_error {
	bad_lexical_cast(const std::string& str):runtime_error(str) {}
};


template<class T, class U>
typename std::enable_if<!std::is_convertible<U, T>::value, T>::type
lexical_cast(const U& val)
{
	T outval;
	std::stringstream str;
	str << val;
	str >> outval;
	if (str.fail()) throw bad_lexical_cast("Bad lexical cast");
	return outval;
}

#ifdef YURI_WIN
#pragma warning ( push )
// THis seems to bethe only wan to disable C4800
#pragma warning ( disable: 4800)
#endif

template<class T, class U>
typename std::enable_if<std::is_convertible<U, T>::value, T>::type
lexical_cast(const U& val)
{
	return static_cast<T>(val);
}

#ifdef YURI_WIN
#pragma warning ( pop)
#endif

template<class Char, class traits>
bool iequals(const std::basic_string<Char, traits>& a, const std::basic_string<Char, traits>& b)
{
	if (a.size() != b.size()) return false;
	auto ai = a.cbegin();
	auto bi = b.cbegin();
	while (ai!=a.cend()) {
		if (std::toupper(*ai++)!=std::toupper(*bi++)) return false;
	}
	return true;
}
template<class Char, class traits>
bool iequals(const std::basic_string<Char, traits>& a, const Char *b)
{
	return iequals(a, std::basic_string<Char, traits>(b));
}


template<class Char, class traits>
bool iless(const std::basic_string<Char, traits>& a, const std::basic_string<Char, traits>& b)
{
	if (a.size() != b.size()) return a.size() < b.size();
	auto ai = a.cbegin();
	auto bi = b.cbegin();
	while (ai!=a.cend()) {
		const auto& au = std::toupper(*ai++);
		const auto& bu = std::toupper(*bi++);
		if (au < bu) return true;
		if (au > bu) return false;
	}
	return false;
}
template<class Char, class traits>
bool iless(const std::basic_string<Char, traits>& a, const Char *b)
{
	return iless(a, std::basic_string<Char, traits>(b));
}

template<class Type, class Value>
bool contains(const Type& container, const Value& value);


template<class Key, class Type, class Value>
bool contains(const std::map<Key, Type>& container, const Value& value)
{
	return container.find(value) != container.end();
}

template<class Type, class Value>
bool contains(const Type& container, const Value& value)
{
	using std::begin;
	using std::end;
	return std::find(begin(container), end(container), value) != end(container);
}

template<typename T, typename T2>
T clip_value(T value, T2 min_value, T2 max_value)
{
	return std::min<T>(std::max<T>(value, min_value), max_value);
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
next_power_2(T value)
{
	if (value < 1) return 1;
	--value;
	for (auto i = 0; i < std::numeric_limits<T>::digits; ++i) {
		value |= value >> i;
	}
	return ++value;
}

template<class T>
class SingletonBase: public T {
public:
	static T& get_instance() {
		static T instance;
		return instance;
	}

private:
	SingletonBase() {};
	virtual ~SingletonBase() noexcept {}
};
}


#endif /* UTILS_H_ */
