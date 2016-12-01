/*!
 * @file 		WebPageGenerator.h
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		07.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#ifndef SRC_MODULES_WEBSERVER_WEBPAGEGENERATOR_H_
#define SRC_MODULES_WEBSERVER_WEBPAGEGENERATOR_H_
#include "common_types.h"
#include <vector>
#include <string>
namespace yuri {
namespace webserver {

struct web_meta_t {
    std::string name;
    std::string content;
};

struct webpage_t {
    std::string             title;
    std::vector<web_meta_t> metas;
    std::string             body;
    bool                    default_footer;
};

std::string prepare_response_header(http_code code);

std::string get_page_content(webpage_t);
webpage_t   get_default_page_stub();
response_t get_default_response(http_code code, const std::string& reason = {});
response_t get_redirect_response(http_code code, const std::string& location);

url_t parse_url(const std::string& uri, const std::string& host = {});

/* ****************************************************
 *                      Tags                          *
 **************************************************** */
namespace tag {
using str_str_map = std::map<std::string, std::string>;
std::string doctype();

std::string gen_tag(const std::string& tag, const std::string& text);
std::string gen_inline_tag(const std::string& tag, const std::string& text, str_str_map = str_str_map{});
std::string gen_empty_tag(const std::string& tag);

std::string indent(const std::string& text, const std::string& ind = "\t");

std::string html(const std::string& text);
std::string head(const std::string& text);
std::string body(const std::string& text);
std::string meta(const web_meta_t& meta);
std::string title(const std::string& text);

std::string header(const std::string& text);
std::string center(const std::string& text);
std::string small(const std::string& text);
std::string anchor(const std::string& text);

std::string line_break();
}
}
}

#endif /* SRC_MODULES_WEBSERVER_WEBPAGEGENERATOR_H_ */
