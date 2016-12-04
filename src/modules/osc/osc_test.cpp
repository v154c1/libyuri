/*!
 * @file 		osc_test.cpp
 * @author 		Zdenek Travnicek
 * @date		15.03.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "tests/catch.hpp"
#include "yuri/core/utils/irange.h"
#include "OSC.h"
namespace yuri {
namespace osc {

namespace {
	const std::string test_name{"/test"};

	const std::vector<char> test_string1 = {'/', 't', 'e', 's', 't', 0, 0, 0, ',', 's', 0, 0, 'a', 'h', 'o', 'j', 0,0,0,0};
	const std::vector<char> test_string2 = {'/', 't', 'e', 's', 't', 0, 0, 0, ',', 'i', 0, 0, 0, 0, 2, 37};
	const std::vector<char> test_string3 = {'/', 't', 'e', 's', 't', 0, 0, 0, ',', 'f', 0, 0, 0x3e, 0x40, 0, 0};
	const std::vector<char> test_string4t = {'/', 't', 'e', 's', 't', 0, 0, 0, ',', 'T', 0, 0};
	const std::vector<char> test_string4f = {'/', 't', 'e', 's', 't', 0, 0, 0, ',', 'F', 0, 0};
	const std::vector<char> test_string5 = {'/', 't', 'e', 's', 't', 0, 0, 0, ',', 'N', 0, 0};
	const std::vector<char> test_string6 = {'/', 't', 'e', 's', 't', 0, 0, 0, ',', 'T', 'F', 0};
	const std::vector<char> test_string7 = {'/', 't', 'e', 's', 't', 0, 0, 0, ',', 'i', 'f', 0, 0, 0, 2, 37, 0x3e, 0x40, 0, 0};

	const std::string test_value1 {"ahoj"};
	const int32_t test_value2 {549};
	const float test_value3 {0.1875};
	const bool test_value4t {true};
	const bool test_value4f {false};

	const auto test_value6 = 		std::vector<event::pBasicEvent>{
					std::make_shared<event::EventBool>(test_value4t),
					std::make_shared<event::EventBool>(test_value4f),
			};
	const auto test_value7 = 		std::vector<event::pBasicEvent>{
					std::make_shared<event::EventInt>(test_value2),
					std::make_shared<event::EventDouble>(test_value3),
			};

	bool cmp(const std::vector<char>& v1, const std::string& s1) {
		if (s1.size() != v1.size()) return false;
		for (auto i: irange(0, v1.size())) {
			if (v1[i] != s1[i]) return false;
		}
		return true;
	}
}

TEST_CASE( "OSC Encoding", "[module]" ) {

#define TEST_ENCODING(name, ev_type, value, output)  \
	SECTION(name) { \
		auto osc_string = encode_osc(test_name, std::make_shared<ev_type>(value),false); \
		REQUIRE( output.size() == osc_string.size() );\
		REQUIRE( cmp(output, osc_string) );\
	}

	TEST_ENCODING("string",	event::EventString,	test_value1,	test_string1)
	TEST_ENCODING("int", 	event::EventInt, 	test_value2, 	test_string2)
	TEST_ENCODING("double",	event::EventDouble,	test_value3,	test_string3)
	TEST_ENCODING("true",	event::EventBool, 	test_value4t,	test_string4t)
	TEST_ENCODING("false",	event::EventBool,	test_value4f,	test_string4f)
	TEST_ENCODING("bang",	event::EventBang, , 				test_string5)

	TEST_ENCODING("vector bool",		event::EventVector, test_value6,	test_string6)
	TEST_ENCODING("vector int/float",	event::EventVector, test_value7,	test_string7)
}

namespace {
template<class Container>
named_event parse_vec(const Container& cont, log::Log& l)
{
	auto b = cont.begin();
	auto e = cont.end();
	return parse_packet(b, e, l);
}




}

TEST_CASE( "OSC Decoding", "[module]" ) {
	std::stringstream ss;
	log::Log l(ss);

#define TEST_DECODING_INNER(input, ev_type) \
		auto event_p = parse_vec(input, l); \
		REQUIRE( std::get<0>(event_p) == test_name ); \
		auto event_vec = std::get<1>(event_p); \
		REQUIRE( event_vec.size() == 1 ); \
		REQUIRE( event_vec[0]->get_type() == ev_type );

#define TEST_DECODING(name, input, ev_type, val) \
	SECTION(name) {\
		TEST_DECODING_INNER(input, ev_type) \
		REQUIRE( event::lex_cast_value<std::remove_cv<decltype(val)>::type>(event_vec[0]) ==  val); \
	}
#define TEST_DECODING_NOVAL(name, input, ev_type) \
	SECTION(name) {\
		TEST_DECODING_INNER(input, ev_type)\
	}

#define TEST_DECODING_INNER_VECTOR2(input, ev_type1, ev_type2) \
	auto event_p = parse_vec(input, l); \
	REQUIRE( std::get<0>(event_p) == test_name ); \
	auto event_vec = std::get<1>(event_p); \
	REQUIRE( event_vec.size() == 2 ); \
	REQUIRE( event_vec[0]->get_type() == ev_type1 ); \
	REQUIRE( event_vec[1]->get_type() == ev_type2 );

#define TEST_DECODING_VECTOR2(name, input, ev_type1, val1, ev_type2, val2) \
	SECTION(name) {\
		TEST_DECODING_INNER_VECTOR2(input, ev_type1, ev_type2) \
		REQUIRE( event::lex_cast_value<std::remove_cv<decltype(val1)>::type>(event_vec[0]) ==  val1); \
		REQUIRE( event::lex_cast_value<std::remove_cv<decltype(val2)>::type>(event_vec[1]) ==  val2); \
	}

	TEST_DECODING("string",		test_string1,	event::event_type_t::string_event,	test_value1)
	TEST_DECODING("int",		test_string2,	event::event_type_t::integer_event,	test_value2)
	TEST_DECODING("double",		test_string3,	event::event_type_t::double_event,	test_value3)
	TEST_DECODING("true",		test_string4t,	event::event_type_t::boolean_event,	test_value4t)
	TEST_DECODING("false",		test_string4f,	event::event_type_t::boolean_event,	test_value4f)

	TEST_DECODING_NOVAL("bang",	test_string5,	event::event_type_t::bang_event)

	TEST_DECODING_VECTOR2("vector bool",test_string6,	event::event_type_t::boolean_event, test_value4t, event::event_type_t::boolean_event, test_value4f)
	TEST_DECODING_VECTOR2("vector i/f",	test_string7,	event::event_type_t::integer_event, test_value2, event::event_type_t::double_event, test_value3)



}
}
}
