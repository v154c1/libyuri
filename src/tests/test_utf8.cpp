/*!
 * @file 		test_utf8.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		1. 12. 2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2016
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "catch.hpp"
#include "yuri/core/utils/utf8.h"

namespace yuri {
namespace core {

namespace {
std::vector<std::tuple<char32_t, std::string>> test_cases = {
    //     {0, "\0"},
    std::make_tuple(0x20, "\x20"),
    std::make_tuple('A', "A"),
    std::make_tuple(0x7f, "\x7f"),
    std::make_tuple(0x80, "\xc2\x80"),
    std::make_tuple(0xff, "\xc3\xbf"),
    std::make_tuple(0x7ff, "\xdf\xbf"),
    std::make_tuple(0x800, "\xe0\xa0\x80"),
    std::make_tuple(0x732b, "\xe7\x8c\xab"),
    std::make_tuple(0xffff, "\xef\xbf\xbf"),
    std::make_tuple(0x10000, "\xf0\x90\x80\x80"),
    std::make_tuple(0x1f600, "\xf0\x9f\x98\x80"),
    std::make_tuple(0x10ffff, "\xf4\x8f\xbf\xbf"),
};
}

TEST_CASE("utf8", "")
{
    SECTION("decode hex")
    {
        for (auto i = 0; i < 10; ++i) {
            REQUIRE(i == utils::decode_hex('0' + i));
        }
        for (auto i = 0; i < 6; ++i) {
            REQUIRE((10 + i) == utils::decode_hex('a' + i));
            REQUIRE((10 + i) == utils::decode_hex('A' + i));
        }
    }
    SECTION("unicode")
    {
        std::string scratch;
        SECTION("sizes")
        {
            for (const auto& t : test_cases) {
                const auto& unicode      = std::get<0>(t);
                const auto& str          = std::get<1>(t);
                const auto  needed_bytes = utils::utf8_char_len(unicode);
                REQUIRE(str.size() == needed_bytes);
                scratch.resize(needed_bytes);
                const size_t used_bytes = utils::unicode_to_utf8(unicode, &scratch[0]);
                REQUIRE(used_bytes == needed_bytes);
                REQUIRE(scratch == str);

                const auto unicode2 = utils::utf8_to_unicode(&str[0], str.size());
                REQUIRE(std::get<0>(unicode2) == unicode);
                REQUIRE(static_cast<size_t>(std::get<1>(unicode2)) == needed_bytes);
            }
        }
    }
}
}
}
