/*!
 * @file 		utf8.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		1. 12. 2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SRC_YURI_CORE_UTILS_UTF8_H_
#define SRC_YURI_CORE_UTILS_UTF8_H_

#include <tuple>

namespace yuri {
namespace utils {

constexpr size_t max_utf8_value = 0x80000000;

/*!
 * Decodes a single HEX digit
 * @param c a digit to decode
 * @return decoded value of the digit, or 0 for invalid output
 */
inline int decode_hex(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    return 0;
}

/*!
 * Calculates number of bytes needed to encode given unicode character
 * @param unicode Input unicode character
 * @return number of bytes
 */
size_t utf8_char_len(char32_t unicode)
{
    return (unicode < 0x80) ? 1 : (unicode < 0x800) ? 2 : (unicode < 0x10000) ? 3 : (unicode < 0x200000) ? 4 : (unicode < 0x400000) ? 5 : 6;
}

namespace detail {
template <int bytes>
/*!
 * Helepr function storing last 6 * @em bytes bits into a buffer
 * @param character Input character
 * @param str output buffer
 * @return number of bytes written (always exactly @em bytes
 */
int unicode_to_utf8_helper(char32_t character, char* str)
{
    for (int i = 0; i < bytes; ++i) {
        str[i] = 0x80 | static_cast<char>((character >> ((bytes - i - 1) * 6)) & 0x3f);
    }
    return bytes;
}
}
/*!
 * Converts a unicode character to utf8 representation
 * @param unicode Character to encode
 * @param str output buffer to store the string. The BUffer has to have enough space to store the utf8 representation
 * @return number of bytes written
 */
inline int unicode_to_utf8(char32_t unicode, char* str)
{
    if (unicode < 0x80) {
        str[0] = 0x00 | static_cast<char>(unicode);
        return 1;
    } else if (unicode < 0x800) {
        str[0] = 0xc0 | static_cast<char>((unicode >> 6) & 0x1f);
        return 1 + detail::unicode_to_utf8_helper<1>(unicode, str + 1);
    } else if (unicode < 0x10000) {
        str[0] = 0xe0 | static_cast<char>((unicode >> 12) & 0x0f);
        return 1 + detail::unicode_to_utf8_helper<2>(unicode, str + 1);
    } else if (unicode < 0x200000) {
        str[0] = 0xf0 | static_cast<char>((unicode >> 18) & 0x07);
        return 1 + detail::unicode_to_utf8_helper<3>(unicode, str + 1);
    } else if (unicode < 0x4000000) {
        str[0] = 0xf8 | static_cast<char>((unicode >> 24) & 0x03);
        return 1 + detail::unicode_to_utf8_helper<4>(unicode, str + 1);
    } else if (unicode < 0x80000000) {
        str[0] = 0xfc | static_cast<char>((unicode >> 30) & 0x01);
        return 1 + detail::unicode_to_utf8_helper<5>(unicode, str + 1);
    }

    return -1;
}

/*!
 * Helper for left shift of a value of a character
 * @param character Character value to shift
 * @param shift number of bits to shift
 * @return shifted character
 */
inline char32_t shift_char(char32_t character, int shift)
{
    return character << shift;
}

/*!
 * Stateful utf8 decoding.
 * Function take a utf8 byte and previous state in @ unicode_char and @em remaining and returns new state.
 * @param c Input utf8 byte
 * @param unicode_char Partially decoded unicode character.
 * @param remaining number of bytes remaining to get full character.
 * @return Partially decoded character and number of remaining bytes. If the nubmer of remaining is 0, then the character is fully decoded.
 */
inline std::tuple<char32_t, int> utf8_char(char c, char32_t unicode_char, int remaining)
{
    if (!(c & 0x80)) {
        return std::make_tuple(c & 0x7F, 0);
    }
    if ((c & 0xC0) == 0x80) {
        return std::make_tuple(unicode_char | shift_char(c & 0x3F, (remaining - 1) * 6), remaining - 1);
    }
    if ((c & 0xE0) == 0xC0) {
        return std::make_tuple(shift_char(c & 0x1F, 6), 1);
    }
    if ((c & 0xF0) == 0xE0) {
        return std::make_tuple(shift_char(c & 0x0F, 12), 2);
    }
    if ((c & 0xF8) == 0xF0) {
        return std::make_tuple(shift_char(c & 0x07, 18), 3);
    }
    return std::make_tuple(c & 0x7F, 0);
}

/*!
 * Decodes a sequence of utf8 bytes into a single unicode character.
 * @param str input buffer
 * @param size size of input buffer
 * @return a tuple, containing decoded character and number of used characters
 */
inline std::tuple<char32_t, int> utf8_to_unicode(const char* str, size_t size)
{
    char32_t unicode   = 0;
    int      remaining = 0;
    size_t   index     = 0;
    while (index < size) {
        std::tie(unicode, remaining) = utf8_char(str[index++], unicode, remaining);
        if (!remaining)
            break;
    }
    return std::make_tuple(unicode, index);
}
}
}

#endif /* SRC_YURI_CORE_UTILS_UTF8_H_ */
