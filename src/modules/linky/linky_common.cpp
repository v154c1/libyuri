/*!
 * @file 		linky_common.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		26. 9. 2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file
 * doc/LICENSE
 *
 */

#include "linky_common.h"
#include "curl/curl.h"
#include <memory>
#include <functional>
#include <iostream>

namespace yuri {
namespace linky {

using curl_ptr_t = std::unique_ptr<CURL, std::function<void(CURL*)>>;

namespace {

curl_ptr_t init_curl()
{
    return curl_ptr_t(curl_easy_init(), [](CURL* c) { curl_easy_cleanup(c); });
}

size_t yuri_write_callback(const char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto& data = *reinterpret_cast<std::string*>(userdata);
    data.insert(data.end(), ptr, ptr + size * nmemb);
    return size * nmemb;
}

size_t yuri_read_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto& data      = *reinterpret_cast<std::string*>(userdata);
    auto  copybytes = std::min(size * nmemb, data.size());
    std::copy(data.begin(), data.begin() + copybytes, ptr);
    if (copybytes < data.size()) {
        data.erase(0, copybytes);
    }
    return copybytes;
}

void try_execute(CURLcode c, const std::string& message)
{
    if (c != CURLE_OK)
        throw std::runtime_error(message);
}
}

std::string download_url(const std::string& url, const std::string& api_key)
{
    auto        curl = init_curl();
    std::string data;
    long        http_code = 0;
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L), "Failed to disable ssl verification");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, yuri_write_callback), "Failed to set an option");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, reinterpret_cast<void*>(&data)), "Failed to set an option");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str()), "Failed to set an option");
    struct curl_slist* headers = nullptr;
    const auto         key     = "api-key: " + api_key;
//    std::cout << "header::: " << key << " :::\n";
    headers = curl_slist_append(headers, key.c_str());
    headers = curl_slist_append(headers, "aaa: bbb");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers), "Failed to set headers");
//    try_execute(curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1L), "faield to set verbose");
    try_execute(curl_easy_perform(curl.get()), "Failed to perform");
    curl_slist_free_all(headers);
    try_execute(curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code), "Failed to obtain return code");
    if (http_code != 200) {
        // We simply discard all received data in case of an error...
//        std::cout << data << "\n";
        data.clear();
    }
    return data;
}

std::string upload_json(const std::string& url, std::string data_in, const std::string& api_key)
{
    auto        curl = init_curl();
    std::string data;
    long        http_code = 0;
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L), "Failed to disable ssl verification");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, yuri_write_callback), "Failed to set an option");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, reinterpret_cast<void*>(&data)), "Failed to set an option");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str()), "Failed to set an option");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_PUT, 1L), "Failed to set PUT");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, yuri_read_callback), "Failed to set an option");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_READDATA, reinterpret_cast<void*>(&data_in)), "Failed to set an option");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, data_in.size()), "Failed to set an option");
//    try_execute(curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1L), "faield to set verbose");
    struct curl_slist* headers = nullptr;
    headers                    = curl_slist_append(headers, "Accept: application/json");
    headers                    = curl_slist_append(headers, "Content-Type: application/json");
    headers                    = curl_slist_append(headers, "charsets: utf-8");
    auto key                   = "api-key: " + api_key;
    headers                    = curl_slist_append(headers, key.c_str());
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers), "Failed to set headers");
    try_execute(curl_easy_setopt(curl.get(), CURLOPT_INFILESIZE_LARGE,
                         static_cast<curl_off_t>(data_in.size())),"failed to set size");
    // try_execute(curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,
    // data_in.c_str()), "Failed to set POST data");
    try_execute(curl_easy_perform(curl.get()), "Failed to perform");
    curl_slist_free_all(headers);
    try_execute(curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code), "Failed to obtain return code");
    if (http_code != 200) {
        // We simply discard all received data in case of an error...
        data.clear();
    }
    return data;
}
}
}
