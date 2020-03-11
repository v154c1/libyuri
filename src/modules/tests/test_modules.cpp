//
// Created by neneko on 11.03.20.
//

#include "tests/catch.hpp"
//#include "../crop/Crop.h"
#include "yuri/core/thread/builder_utils.h"
#include "yuri/core/thread/IOThreadGenerator.h"
#include "yuri/core/thread/IOFilter.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include <iostream>

namespace yuri {
    namespace test {
        log::Log log(std::cout);

        const resolution_t test_res{640, 480};
        const format_t test_fmt = core::raw_format::rgb24;

        template<class T>
        std::shared_ptr<T> empty_class(log::Log &l, const std::string &class_name, const core::Parameters &p) {
            auto params = IOThreadGenerator::get_instance().configure(class_name);
            params.merge(p);
//        params["_node_name"]=name;
            auto m = IOThreadGenerator::get_instance().generate(class_name, l, core::pwThreadBase{}, params);
            return std::dynamic_pointer_cast<T>(m);
        }


        std::shared_ptr<core::RawVideoFrame> empty_frame(format_t fmt, resolution_t res) {
            return core::RawVideoFrame::create_empty(fmt, res);
        }

        void test_filter_class_inner(std::shared_ptr<core::IOFilter> &cls, resolution_t src_res, format_t src_fmt,
                                     resolution_t res, format_t fmt) {
            auto f1 = empty_frame(src_fmt, src_res);

            auto f2 = std::dynamic_pointer_cast<core::RawVideoFrame>(cls->simple_single_step(f1));
            REQUIRE(f2);
            REQUIRE(f2->get_format() == fmt);
            REQUIRE(f2->get_resolution() == res);
            REQUIRE(f1->get_timestamp() == f2->get_timestamp());
        }

        void
        test_filter_class(const core::Parameters &p, const std::string &name,
                          resolution_t res, format_t fmt  = test_fmt) {
            auto cls = empty_class<core::IOFilter>(log, name, p);
            REQUIRE(cls);
            test_filter_class_inner(cls, test_res, test_fmt, res, fmt);

        }

        TEST_CASE("modules") {
            core::builder::load_builtin_modules(log);
            SECTION("crop") {
                core::Parameters p;
                geometry_t g{320, 240, 0, 0};
                p["geometry"] = g;
                test_filter_class(p, "crop", g.get_resolution());
            }

            SECTION("pad") {
                core::Parameters p;
                resolution_t r{960, 360};
                p["resolution"] = r;
                test_filter_class(p, "pad", r);
            }

            SECTION("scale") {
                core::Parameters p;
                resolution_t r{960, 360};
                p["resolution"] = r;
                test_filter_class(p, "scale", r);
            }
            SECTION("frame_info") {
//                core::Parameters p;

                test_filter_class({}, "frame_info", test_res);
            }
            SECTION("yuri_convert") {
                using namespace core::raw_format;
                const std::map<std::string, format_t> format_list {
                        {"RGB24", rgb24},
                        {"YUV", yuyv422}
                };
                for (const auto& fmt: format_list) {
                    core::Parameters p;
                    p["format"] = fmt.first;

                    test_filter_class(p, "yuri_convert", test_res, fmt.second);
                }
            }
        }

    }
}