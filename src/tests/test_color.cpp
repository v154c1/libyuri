/*!
 * @file 		test_color.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		4. 8. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "catch.hpp"
#include "yuri/core/utils/color.h"

namespace yuri {
namespace core {

TEST_CASE("color", "")
{
    auto black = color_t::create_rgb(0x00, 0x00, 0x00);
    auto white = color_t::create_rgb(0xFF, 0xFF, 0xFF);
    auto red   = color_t::create_rgb(0xFF, 0x00, 0x00);
    auto green = color_t::create_rgb(0x00, 0xFF, 0x00);
    auto blue  = color_t::create_rgb(0x00, 0x00, 0xFF);

    auto yuv_black = color_t::create_yuv(0x00, 0x7F, 0x7F);
    auto yuv_white = color_t::create_yuv(0xFF, 0x7F, 0x7F);

    auto red16 = color_t::create_rgb16(0xFFFF, 0x0000, 0x0000);
    SECTION("accessors")
    {

        // Default color if black RGB
        REQUIRE(color_t{} == black);

        // Black should be the same in every colorspace
        REQUIRE(black == yuv_black);
        REQUIRE(yuv_black == black);

        REQUIRE(color_t::create_rgb16(0, 0, 0) == black);
        REQUIRE(color_t::create_yuv16(0, 0x7FFF, 0x7FFF) == black);
        REQUIRE(black == color_t::create_rgb16(0, 0, 0));
        REQUIRE(black == color_t::create_yuv16(0, 0x7FFF, 0x7FFF));

        // Exact comparison may fail due to limited precision
        REQUIRE(color_distance_rgb(yuv_white, white) <= 2.01);
        REQUIRE(color_distance_rgb(white, yuv_white) <= 2.01);

        REQUIRE(color_distance_yuv(white, yuv_white) <= 2.01);
        REQUIRE(color_distance_yuv(yuv_white, white) <= 2.01);

        REQUIRE(red == red16);
        //		auto r16 = red;
        //		r16.convert_to_rgb16();
        //		REQUIRE(red16 != r16);
        REQUIRE(color_t::create_rgb16(0xFF00, 0, 0) == red);
        REQUIRE(red16 != red);

        REQUIRE(color_t::create_rgba(0xFF, 0, 0, 0xFF) == red);
    }

    SECTION("parsing")
    {
        REQUIRE(lexical_cast<std::string>(black) == "#000000");
        REQUIRE(lexical_cast<std::string>(red) == "#ff0000");
        REQUIRE(lexical_cast<std::string>(green) == "#00ff00");
        REQUIRE(lexical_cast<std::string>(blue) == "#0000ff");

        REQUIRE(lexical_cast<color_t>("#000000") == black);
        REQUIRE(lexical_cast<color_t>("#FF0000") == red);
        REQUIRE(lexical_cast<color_t>("#00FF00") == green);
        REQUIRE(lexical_cast<color_t>("#0000FF") == blue);
        REQUIRE(lexical_cast<color_t>("#ffffff") == white);

        REQUIRE(lexical_cast<std::string>(yuv_black) == "#@007f7f");
        REQUIRE(lexical_cast<color_t>("#@007F7F") == yuv_black);

        REQUIRE(lexical_cast<color_t>("#abcdef") == color_t::create_rgb(0xab, 0xcd, 0xef));
        REQUIRE(lexical_cast<color_t>("##abcdef123456") == color_t::create_rgb16(0xabcd, 0xef12, 0x3456));
        REQUIRE(lexical_cast<color_t>("#@abcdef") == color_t::create_yuv(0xab, 0xcd, 0xef));
        REQUIRE(lexical_cast<color_t>("##@abcdef123456") == color_t::create_yuv16(0xabcd, 0xef12, 0x3456));
    }
}
}
}
