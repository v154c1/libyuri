/*!
 * @file 		test_irange.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		16. 3. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */


#include "catch.hpp"
#include "yuri/core/utils/irange.h"


namespace yuri{
TEST_CASE( "irange", "[irange]" ) {

	int i = 0;
	for ([[maybe_unused]] auto idx: irange(0,0)) { ++i; }
	REQUIRE( i == 0);
	for ([[maybe_unused]] auto idx: irange(0,10)) { ++i; }
	REQUIRE( i == 10);

	i = 0;
	for ([[maybe_unused]] auto idx: irange(100,110)) { ++i; }
	REQUIRE( i == 10);

	auto r = irange(10, 20);
	REQUIRE(*(r.begin()) == 10);
	REQUIRE(*(r.end()) == 20);

}


}
