#include "catch.hpp"
#include "yuri/log/Log.h"


namespace yuri {
    struct string_output : public log::generic_out_stream<char> {

        void put_message(const log::LogMessageInfo &info, const sstream_t &sstream) override {
            result = sstream.str();
            last_level = info.level;
        }

        std::string result;
        log::debug_flags last_level;
    };

    TEST_CASE("log") {
        SECTION("to-sstream") {
            std::stringstream sstr;
            {
                log::Log log0(sstr);
                log0.set_flags(log::info | log::show_level);
                log0[log::info] << "TEST";
            }
            REQUIRE(!sstr.str().empty());
        }
        SECTION("custom") {
            auto so = std::make_shared<string_output>();
            {
                log::Log log0(so);
                log0.set_flags(log::info | log::show_level);
                log0[log::info] << "TEST";
            }
            REQUIRE(so->result == "TEST");
            REQUIRE(so->last_level == log::info);
        }
    }
}