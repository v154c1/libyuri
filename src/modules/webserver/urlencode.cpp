/*!
 * @file 		urlencode.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		30. 11. 2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */
#include "urlencode.h"
#include "yuri/core/utils/utf8.h"
#include <unordered_map>
#include <cassert>
#include <boost/regex.hpp>
#include <iostream>
namespace yuri {
namespace webserver {
namespace {
std::unordered_map<std::string, char32_t> named_entities = {
    { "amp", 38 },      { "lt", 60 },       { "gt", 62 },       { "Agrave", 192 },   { "Aacute", 193 },  { "Acirc", 194 },   { "Atilde", 195 },
    { "Auml", 196 },    { "Aring", 197 },   { "AElig", 198 },   { "Ccedil", 199 },   { "Egrave", 200 },  { "Eacute", 201 },  { "Ecirc", 202 },
    { "Euml", 203 },    { "Igrave", 204 },  { "Iacute", 205 },  { "Icirc", 206 },    { "Iuml", 207 },    { "ETH", 208 },     { "Ntilde", 209 },
    { "Ograve", 210 },  { "Oacute", 211 },  { "Ocirc", 212 },   { "Otilde", 213 },   { "Ouml", 214 },    { "Oslash", 216 },  { "Ugrave", 217 },
    { "Uacute", 218 },  { "Ucirc", 219 },   { "Uuml", 220 },    { "Yacute", 221 },   { "THORN", 222 },   { "szlig", 223 },   { "agrave", 224 },
    { "aacute", 225 },  { "acirc", 226 },   { "atilde", 227 },  { "auml", 228 },     { "aring", 229 },   { "aelig", 230 },   { "ccedil", 231 },
    { "egrave", 232 },  { "eacute", 233 },  { "ecirc", 234 },   { "euml", 235 },     { "igrave", 236 },  { "iacute", 237 },  { "icirc", 238 },
    { "iuml", 239 },    { "eth", 240 },     { "ntilde", 241 },  { "ograve", 242 },   { "oacute", 243 },  { "ocirc", 244 },   { "otilde", 245 },
    { "ouml", 246 },    { "oslash", 248 },  { "ugrave", 249 },  { "uacute", 250 },   { "ucirc", 251 },   { "uuml", 252 },    { "yacute", 253 },
    { "thorn", 254 },   { "yuml", 255 },    { "nbsp", 160 },    { "iexcl", 161 },    { "cent", 162 },    { "pound", 163 },   { "curren", 164 },
    { "yen", 165 },     { "brvbar", 166 },  { "sect", 167 },    { "uml", 168 },      { "copy", 169 },    { "ordf", 170 },    { "laquo", 171 },
    { "not", 172 },     { "shy", 173 },     { "reg", 174 },     { "macr", 175 },     { "deg", 176 },     { "plusmn", 177 },  { "sup2", 178 },
    { "sup3", 179 },    { "acute", 180 },   { "micro", 181 },   { "para", 182 },     { "cedil", 184 },   { "sup1", 185 },    { "ordm", 186 },
    { "raquo", 187 },   { "frac14", 188 },  { "frac12", 189 },  { "frac34", 190 },   { "iquest", 191 },  { "times", 215 },   { "divide", 247 },
    { "forall", 8704 }, { "part", 8706 },   { "exist", 8707 },  { "empty", 8709 },   { "nabla", 8711 },  { "isin", 8712 },   { "notin", 8713 },
    { "ni", 8715 },     { "prod", 8719 },   { "sum", 8721 },    { "minus", 8722 },   { "lowast", 8727 }, { "radic", 8730 },  { "prop", 8733 },
    { "infin", 8734 },  { "ang", 8736 },    { "and", 8743 },    { "or", 8744 },      { "cap", 8745 },    { "cup", 8746 },    { "int", 8747 },
    { "there4", 8756 }, { "sim", 8764 },    { "cong", 8773 },   { "asymp", 8776 },   { "ne", 8800 },     { "equiv", 8801 },  { "le", 8804 },
    { "ge", 8805 },     { "sub", 8834 },    { "sup", 8835 },    { "nsub", 8836 },    { "sube", 8838 },   { "supe", 8839 },   { "oplus", 8853 },
    { "otimes", 8855 }, { "perp", 8869 },   { "sdot", 8901 },   { "Alpha", 913 },    { "Beta", 914 },    { "Gamma", 915 },   { "Delta", 916 },
    { "Epsilon", 917 }, { "Zeta", 918 },    { "Eta", 919 },     { "Theta", 920 },    { "Iota", 921 },    { "Kappa", 922 },   { "Lambda", 923 },
    { "Mu", 924 },      { "Nu", 925 },      { "Xi", 926 },      { "Omicron", 927 },  { "Pi", 928 },      { "Rho", 929 },     { "Sigma", 931 },
    { "Tau", 932 },     { "Upsilon", 933 }, { "Phi", 934 },     { "Chi", 935 },      { "Psi", 936 },     { "Omega", 937 },   { "alpha", 945 },
    { "beta", 946 },    { "gamma", 947 },   { "delta", 948 },   { "epsilon", 949 },  { "zeta", 950 },    { "eta", 951 },     { "theta", 952 },
    { "iota", 953 },    { "kappa", 954 },   { "lambda", 955 },  { "mu", 956 },       { "nu", 957 },      { "xi", 958 },      { "omicron", 959 },
    { "pi", 960 },      { "rho", 961 },     { "sigmaf", 962 },  { "sigma", 963 },    { "tau", 964 },     { "upsilon", 965 }, { "phi", 966 },
    { "chi", 967 },     { "psi", 968 },     { "omega", 969 },   { "thetasym", 977 }, { "upsih", 978 },   { "piv", 982 },     { "OElig", 338 },
    { "oelig", 339 },   { "Scaron", 352 },  { "scaron", 353 },  { "Yuml", 376 },     { "fnof", 402 },    { "circ", 710 },    { "tilde", 732 },
    { "ensp", 8194 },   { "emsp", 8195 },   { "thinsp", 8201 }, { "zwnj", 8204 },    { "zwj", 8205 },    { "lrm", 8206 },    { "rlm", 8207 },
    { "ndash", 8211 },  { "mdash", 8212 },  { "lsquo", 8216 },  { "rsquo", 8217 },   { "sbquo", 8218 },  { "ldquo", 8220 },  { "rdquo", 8221 },
    { "bdquo", 8222 },  { "dagger", 8224 }, { "Dagger", 8225 }, { "bull", 8226 },    { "hellip", 8230 }, { "permil", 8240 }, { "prime", 8242 },
    { "Prime", 8243 },  { "lsaquo", 8249 }, { "rsaquo", 8250 }, { "oline", 8254 },   { "euro", 8364 },   { "trade", 8482 },  { "larr", 8592 },
    { "uarr", 8593 },   { "rarr", 8594 },   { "darr", 8595 },   { "harr", 8596 },    { "crarr", 8629 },  { "lceil", 8968 },  { "rceil", 8969 },
    { "lfloor", 8970 }, { "rfloor", 8971 }, { "loz", 9674 },    { "spades", 9824 },  { "clubs", 9827 },  { "hearts", 9829 }, { "diams", 9830 },

};
}
std::string decode_urlencoded(std::string str)
{
    for (auto start = 0u; start < str.size(); ++start) {
        if (str[start] == '%' && start < (str.size() - 2)) {
            int val    = utils::decode_hex(str[start + 1]) << 4 | utils::decode_hex(str[start + 2]);
            str[start] = static_cast<char>(val & 0xff);
            str.erase(start + 1, 2);
        }
    }
    return str;
}

namespace {
template <class Iter>
char32_t parse_numeric_entity(Iter start, Iter end)
{
    char32_t val = 0;
    while (start < end) {
        val = val * 10 + utils::decode_hex(*start++);
    }
    return val;
}

char32_t parse_named_entity(const std::string& name)
{
    auto it = named_entities.find(name);
    if (it != named_entities.end()) {
        return it->second;
    }
    return 0;
}
}

std::string decode_html_entities(std::string str)
{
    boost::regex  entity("&([#a-zA-Z][0-9a-zA-Z]+);");
    boost::smatch m;
    size_t        start_index = 0;
    while (regex_search(str.cbegin() + start_index, str.cend(), m, entity, boost::match_default)) {
        if (!m[1].matched)
            break;
        char32_t unicode = 0;
        if (*(m[1].first) == '#') {
            // Numeric entity
            unicode = parse_numeric_entity(m[1].first + 1, m[1].second);
        } else {
            // Named entity
            unicode = parse_named_entity(m[1].str());
        }
        if (unicode) {
            const auto bytes_needed = utils::utf8_char_len(unicode);
            // The '+ 2' is for leading '&' and trailing ';'
            size_t bytes_available = std::distance(m[1].first, m[1].second) + 2;
            // Keep the result as indices to original string rather than iterators
            // as the iterators would be invalidated if the string needs to be expanded
            const auto match_start = std::distance(str.cbegin(), m[1].first);
            auto       match_end   = std::distance(str.cbegin(), m[1].second);
            if (bytes_needed > bytes_available) {
                // Expend the strin, if there's not enough space
                str.insert(match_end, bytes_needed - bytes_available, '$');
                match_end       = match_end + bytes_needed - bytes_available;
                bytes_available = bytes_needed;
            }
            const auto replace_size = utils::unicode_to_utf8(unicode, &str[match_start - 1]);
            str.erase(match_start + replace_size - 1, bytes_available - bytes_needed);
            start_index = match_start + replace_size - 1;
        } else {
            // There was a hit, but it didn't produce valid HTML entity. So we have to skip the whole match
            start_index = std::distance(str.cbegin(), m[1].second) + 1;
        }
    }
    return str;
}
}
}
