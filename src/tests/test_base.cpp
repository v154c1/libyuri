/*
 * test_base.cpp
 *
 *  Created on: 15. 3. 2015
 *      Author: neneko
 */

#include "catch.hpp"
#include "yuri/log/Log.h"


TEST_CASE( "logging basics", "[log]" ) {
	using namespace yuri;
	std::ostringstream ss;
    log::Log l(ss);
    l.set_flags(log::info);
    l[log::info] << "Hello world " << 5;
    const auto s = ss.str();
    // Delete the log id
    const auto s2 = s.substr(s.find(':'));
    REQUIRE( s2 == ": Hello world 5\n");
}

TEST_CASE( "geometry types", "[resolution_t, geometry_t, coordinates_t]" ) {
	using namespace yuri;
	SECTION("invalid resolution/geometry") {
		auto r0 = resolution_t{0,0};
		REQUIRE( static_cast<bool>(r0) == false);
		auto g0 = geometry_t{0,0,0,0};
		REQUIRE( static_cast<bool>(g0) == false);
	}
	SECTION("valid resolution/geometry") {
		auto r0 = resolution_t{800,600};
		REQUIRE( static_cast<bool>(r0) == true);
		auto g0 = geometry_t{800,600,0,0};
		REQUIRE( static_cast<bool>(g0) == true);
	}
	SECTION("resolution_t ang geometry_t conversions") {
		const auto r0 = resolution_t{800,600};
		const auto g0 = geometry_t{800,600,0,0};
		const auto g1 = geometry_t{800,600,100,100};
		REQUIRE( r0 == g0.get_resolution() );
		REQUIRE( r0 == g1.get_resolution() );
//		REQUIRE( r0.get_geometry() == g0 );
//		REQUIRE( r0.get_geometry() != g1 );
		const auto g2 = intersection(g0, g1);
		const auto r1 = resolution_t{700,500};
		REQUIRE( r1 == g2.get_resolution() );

	}

}


TEST_CASE( "fractions", "[fraction_t]" ) {
	using namespace yuri;
	auto f0 = fraction_t{0,0};
	REQUIRE(!f0.valid());
	auto f1 = fraction_t{1,2};
	REQUIRE( f1.get_value() == 0.5 );

//	auto f2 = fraction_t{1,1};
//	REQUIRE( ( f1 + f1 ) == f2 );

}
