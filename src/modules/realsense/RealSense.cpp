/*!
 * @file 		RealSense.cpp
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		25.08.2020
 * @copyright	Institute of Intermedia, CTU in Prague, 2020
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "RealSense.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/utils/irange.h"


namespace yuri {
    namespace realsense {

        namespace {
            std::map<rs2_format, format_t> rs2_formats = {
                    {RS2_FORMAT_RGB8, core::raw_format::rgb24},
                    {RS2_FORMAT_Z16,  core::raw_format::depth16}

            };

            format_t get_yuri_format_from_rs2(rs2_format fmt) {
                const auto it = rs2_formats.find(fmt);
                if (it != rs2_formats.end()) {
                    return it->second;
                }
                return core::raw_format::unknown;
            }

            resolution_t get_resolution_from_rs2_frame(const rs2::video_frame &rs2_frame) {
                return {
                        static_cast<dimension_t>(rs2_frame.get_width()),
                        static_cast<dimension_t>(rs2_frame.get_height())};
            }

            core::pRawVideoFrame get_frame_from_rs(const rs2::video_frame &rs2_frame) {
                const auto fmt = get_yuri_format_from_rs2(rs2_frame.get_profile().format());
                if (!fmt) {
                    return {};
                }
                return core::RawVideoFrame::create_empty(fmt, get_resolution_from_rs2_frame(rs2_frame),
                                                         reinterpret_cast<const uint8_t *>(rs2_frame.get_data()),
                                                         rs2_frame.get_data_size());
            }


        }


        IOTHREAD_GENERATOR(RealSense)

        MODULE_REGISTRATION_BEGIN("realsense")
            REGISTER_IOTHREAD("realsense", RealSense)
        MODULE_REGISTRATION_END()

        core::Parameters RealSense::configure() {
            core::Parameters p = core::IOThread::configure();
            p.set_description("RealSense");
            p["clip_distance"]["Distance for removing background"] = -1.0f;
            return p;
        }


        RealSense::RealSense(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters) :
                core::IOThread(log_, parent, 1, 1, std::string("realsense")) {
            IOTHREAD_INIT(parameters)
        }

        RealSense::~RealSense() noexcept {
        }

        void RealSense::run() {
            log[log::info] << "Starting pipeline";
            profile_ = pipeline_.start();
            const auto streams = profile_.get_streams();
            for (const auto &s: streams) {
                log[log::info] << "Stream " << s.stream_index() << ": " << s.stream_name();
            }
            const auto &device = profile_.get_device();
            const auto &sensors = device.query_sensors();
            for (const auto &sensor: sensors) {
                if (const auto depth = sensor.as<const rs2::depth_sensor>()) {
                    depth_scale_ = depth.get_depth_scale();
                }
            }
            log[log::info] << "Depth scale " << depth_scale_;
            // Require aligning to color
            rs2::align align(RS2_STREAM_COLOR);


            core::pFrame frame = pop_frame(0);
            while (still_running()) {
                try {
                    auto frameset = pipeline_.wait_for_frames();
                    const auto aligned = align.process(frameset);
                    auto color = aligned.first(RS2_STREAM_COLOR).as<rs2::video_frame>();
                    auto depth = aligned.first(RS2_STREAM_DEPTH).as<rs2::video_frame>();
//                    log[log::info] << depth.get_profile().format();
                    push_frame(0, remove_bg(color, depth));
                    //push_frame(0, get_frame_from_rs(depth));
//                    push_frame(0, color_frame);

                }
                catch (const rs2::error &e) {
                    log[log::error] << "RS2 Error " << e.what();
                }
                //sleep(get_latency());
            }

        }

        core::pRawVideoFrame RealSense::remove_bg(const rs2::frame &color_frame, const rs2::frame &depth_frame) {
            const auto res = get_resolution_from_rs2_frame(color_frame);
            if (res != get_resolution_from_rs2_frame(depth_frame)) {
                log[log::error] << "Resolution mismatch";
                return {};
            }

            auto frame = core::RawVideoFrame::create_empty(core::raw_format::rgb24, res);

            const auto data_color = reinterpret_cast<const uint8_t *>(color_frame.get_data());
            const auto data_depth = reinterpret_cast<const uint16_t *>(depth_frame.get_data());
            auto data = PLANE_RAW_DATA(frame, 0);

            const auto depth_line_width = res.width;
            const auto color_line_width = res.width * 3;
            const uint16_t max_depth = static_cast<uint16_t>(clip_distance_ / depth_scale_);
            for (const auto line: irange(res.height)) {

                auto color_start = data_color + line * color_line_width;
                auto data_start = data + line * color_line_width;
                const auto depth_start = data_depth + line * depth_line_width;
                std::for_each(depth_start, depth_start + depth_line_width, [&](const uint16_t depth) {
                    if (depth != 0 && depth <= max_depth) {
                        std::copy(color_start, color_start + 3, data_start);
                    } else {
                        std::fill(data_start, data_start + 3, 0);
                    }
                    data_start += 3;
                    color_start += 3;
                });
            }
            return frame;
        }


        bool RealSense::set_param(const core::Parameter &param) {
            if (assign_parameters(param)
                    (clip_distance_, "clip_distance")) {
                return true;
            }
            return core::IOThread::set_param(param);
        }

    } /* namespace realsense */
} /* namespace yuri */
