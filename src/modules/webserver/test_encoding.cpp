/*!
 * @file 		test_encoding.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		1. 12. 2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2016
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */


#include "tests/catch.hpp"
#include "base64.h"
#include "urlencode.h"
#include <vector>
#include <string>
#include <iostream>
namespace yuri {
namespace webserver {

namespace {
std::vector<std::pair<std::string, std::string>> test_cases_base64 = {
		{"A", "QQ=="},//
		{"AB", "QUI="},//
		{"Hello World", "SGVsbG8gV29ybGQ="},//
		{"1234567890abcdefghijklmnopqrstuvxyz$", "MTIzNDU2Nzg5MGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ4eXok"}
};

std::vector<std::pair<std::string, std::string>> test_cases_urlencoding = {
		{"1234567890abcdefghijklmnopqrstuvxyz$", "1234567890abcdefghijklmnopqrstuvxyz$"},//
		{"%$&*@##/", "%25%24%26*%40%23%23%2F"},
};
std::vector<std::pair<std::string, std::string>> test_cases_entities = {
		{"1234567890abcdefghijklmnopqrstuvxyz$", "1234567890abcdefghijklmnopqrstuvxyz$"},//
		{"Ahoj & &", "Ahoj &#38; &amp;"},
		{"Neko\xe7\x8c\xab\xf0\x9f\x98\xb2", "Neko&#29483;&#128562;"},
		{"Neko&#29483&#128562", "Neko&#29483&#128562"},
};


}

TEST_CASE("encoding", "")
{
    SECTION("base64") {
    	for (const auto& t: test_cases_base64) {
    		const auto encoded = base64::encode(t.first);
    		REQUIRE(encoded == t.second);
    		const auto decoded = base64::decode(t.second);
    		REQUIRE(decoded == t.first);
    	}
    }
    SECTION("urlencoding") {
    	for (const auto& t: test_cases_urlencoding) {
			const auto decoded = decode_urlencoded(t.second);
			REQUIRE(decoded == t.first);
		}
    }
    SECTION("html_entities") {
		for (const auto& t: test_cases_entities) {
			const auto decoded = decode_html_entities(t.second);
			REQUIRE(decoded == t.first);
		}
	}

}

}
}

