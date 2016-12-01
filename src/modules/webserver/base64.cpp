/*!
 * @file 		base64.cpp
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		08.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#include "base64.h"
#include <array>
#include <cstdint>
#include <iostream>
namespace yuri {
namespace webserver {
namespace base64 {

namespace {
const std::array<uint8_t, 64> base64_map = { { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                                               'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                                               's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' } };

const std::array<uint8_t, 128> unbase64_map = { {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0   -  7
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 8   - 15
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 16  - 23
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 24  - 31
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 32  - 49
    0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, // 40  - 47  (+, /)
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, // 48  - 55  (0-7)
    0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, // 56  - 63  (8-9, =)
    0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // 64  - 71  (A-G)
    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, // 72  - 79  (H-O)
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, // 80  - 87  (P-W)
    0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 88  - 95  (X-Z)
    0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, // 96  - 103 (a-g)
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, // 104 - 111 (h-o)
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, // 112 - 119 (p-w)
    0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 120 - 127 (x-z)

} };

std::string encode_number(uint32_t num)
{
    std::string text(4, ' ');
    for (int i = 0; i < 4; ++i) {
        text[i] = base64_map[(num & 0xFC0000) >> 18];
        num     = num << 6;
    }
    return text;
}

uint8_t decode_character(uint8_t num)
{
    return unbase64_map[num];
}
}

std::string encode(std::string orig)
{
    int missing = orig.size() % 3;
    int padding = 0;
    if (missing) {
        padding = 3 - missing;
        orig.resize(orig.size() + padding, 0);
    }
    std::string encoded;
    encoded.reserve(4 * orig.size() / 3);

    auto begin = orig.cbegin();
    auto end   = orig.cend();
    while (std::distance(begin, end) >= 3) {
        uint32_t num = (static_cast<uint8_t>(*begin) << 16) + (static_cast<uint8_t>(*(begin + 1)) << 8) + static_cast<uint8_t>(*(begin + 2));
        begin += 3;
        auto t = encode_number(num);
        encoded.append(t.cbegin(), t.cend());
    }
    for (int i = 0; i < padding; ++i) {
        encoded[encoded.size() - 1 - i] = '=';
    }

    return encoded;
}

std::string decode(const std::string& encoded)
{
    std::string text;
    text.reserve(3 * encoded.size() / 4);
    auto begin = encoded.cbegin();
    auto end   = encoded.cend();
    while (std::distance(begin, end) >= 4) {
        uint32_t num = 0;
        for (auto i = begin; i < begin + 4; ++i) {
            num = (num << 6) + decode_character(static_cast<uint8_t>(*i));
        }
        text.push_back(static_cast<char>((num & 0xFF0000) >> 16));
        text.push_back(static_cast<char>((num & 0x00FF00) >> 8));
        text.push_back(static_cast<char>((num & 0x0000FF) >> 0));
        begin += 4;
    }
    if (encoded.size() > 4) {
        int padding = 0;
        if (*(begin - 1) == '=') {
            if ((*begin - 2) == '=') {
                padding = 2;
            } else {
                padding = 1;
            }
        }
        if (padding)
            text.resize(text.size() - padding);
    }
    return text;
}
}
}
}
