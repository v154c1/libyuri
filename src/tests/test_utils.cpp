/*!
 * @file 		test_utils.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		4. 4. 2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "catch.hpp"
#include "yuri/core/utils.h"

namespace yuri {
TEST_CASE("power-of-2") {
	SECTION("int") {
		REQUIRE(next_power_2(-1) == 1);
		REQUIRE(next_power_2(0) == 1);
		REQUIRE(next_power_2(1) == 1);
		REQUIRE(next_power_2(2) == 2);
		REQUIRE(next_power_2(3) == 4);
		REQUIRE(next_power_2(4) == 4);
		REQUIRE(next_power_2(9) == 16);
		REQUIRE(next_power_2(16) == 16);
		REQUIRE(next_power_2(17) == 32);
		REQUIRE(next_power_2(0xFFFF) == 0x010000);
	}
	SECTION("uint") {
		REQUIRE(next_power_2(0u) == 1u);
		REQUIRE(next_power_2(1u) == 1u);
		REQUIRE(next_power_2(2u) == 2u);
		REQUIRE(next_power_2(3u) == 4u);
		REQUIRE(next_power_2(4u) == 4u);
		REQUIRE(next_power_2(9u) == 16u);
		REQUIRE(next_power_2(16u) == 16u);
		REQUIRE(next_power_2(17u) == 32u);
		REQUIRE(next_power_2(0xFFFFu) == 0x010000u);
	}

}


}

