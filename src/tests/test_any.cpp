/*!
 * @file 		test_any.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		17. 9. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "catch.hpp"
#include "yuri/core/utils/any.h"

namespace yuri{
namespace core{


TEST_CASE("any", "" ) {
	SECTION("empty") {
		utils::any empty_any;
		REQUIRE(empty_any.empty());
	}

	SECTION("integer") {
		utils::any any_int(5);
		REQUIRE(!any_int.empty());
		REQUIRE(any_int.get<int>() == 5);
		REQUIRE_THROWS_AS(any_int.get<float>(), std::bad_cast);
		any_int = utils::any{18};
		REQUIRE(any_int.get<int>() == 18);
	}
	const auto str = std::string{"Ahoj"};
	SECTION("string") {

		utils::any any_str{str};
		REQUIRE(!any_str.empty());
		REQUIRE(any_str.get<std::string>() == str);
		REQUIRE_THROWS_AS(any_str.get<int>(), std::bad_cast);
		auto any_str2 = any_str;
		REQUIRE(!any_str2.empty());
		REQUIRE(any_str2.get<std::string>() == str);
		REQUIRE(any_str2.get<std::string>() == any_str.get<std::string>());
		any_str = utils::any{42};
		REQUIRE(any_str.get<int>() == 42);

		any_str = {};
		REQUIRE(any_str.empty());

		any_str = std::move(any_str2);
		REQUIRE(!any_str.empty());
	}

	SECTION("Direct assignment") {
		utils::any any_value;
		REQUIRE(any_value.empty());
		any_value = 5;
		REQUIRE(!any_value.empty());
		REQUIRE(any_value.get<int>() == 5);
		any_value = str;
		REQUIRE_THROWS_AS(any_value.get<int>(), std::bad_cast);
		REQUIRE(any_value.get<std::string>() == str);
	}

	SECTION("Type querying") {
		utils::any any_value;
		REQUIRE(any_value.is<int>() == false);
		REQUIRE(any_value.is<std::string>() == false);

		any_value = 5;
		REQUIRE(any_value.is<int>() == true);
		REQUIRE(any_value.is<std::string>() == false);

		any_value = str;
		REQUIRE(any_value.is<int>() == false);
		REQUIRE(any_value.is<std::string>() == true);

	}


}

}
}






