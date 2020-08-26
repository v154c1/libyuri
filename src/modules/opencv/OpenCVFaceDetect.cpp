/*!
 * @file 		OpenCVFaceDetect.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		04.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "OpenCVFaceDetect.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "opencv2/imgproc/imgproc.hpp"

namespace yuri {
    namespace opencv {


        IOTHREAD_GENERATOR(OpenCVFaceDetect)

        core::Parameters OpenCVFaceDetect::configure() {
            core::Parameters p = core::SpecializedIOFilter<core::RawVideoFrame>::configure();
            p.set_description("OpenCVFaceDetect");
            p["haar_cascade"]["Path to he xml file with HAAR cascade"] = "haarcascade_frontalface_default.xml";
            p["min_face_size"]["Minimal detected face size."] = 0;
            p["max_face_size"]["Maximum detected face size. Set to negative number to disable"] = -1;
            return p;
        }

        namespace {
            const std::string width_event{"width"};
            const std::string height_event{"height"};
            const std::string x_event{"x"};
            const std::string y_event{"y"};

        }

        OpenCVFaceDetect::OpenCVFaceDetect(const log::Log &log_, core::pwThreadBase parent,
                                           const core::Parameters &parameters) :
                core::SpecializedIOFilter<core::RawVideoFrame>(log_, parent, std::string("opencv_facedetection")),
                event::BasicEventProducer(log),
                haar_cascade_file_("haarcascade_frontalface_default.xml") {
            IOTHREAD_INIT(parameters)
            haar_cascade_.load(haar_cascade_file_);
            set_supported_formats({core::raw_format::y8});
        }

        OpenCVFaceDetect::~OpenCVFaceDetect() noexcept {
        }

        core::pFrame OpenCVFaceDetect::do_special_single_step(core::pRawVideoFrame frame) {
            resolution_t res = frame->get_resolution();
            cv::Mat in_mat(res.height, res.width, CV_8UC1, PLANE_RAW_DATA(frame, 0));
            std::vector<cv::Rect> faces;
            cv::Size min_size{static_cast<int>(min_face_size_ * 2), static_cast<int>(min_face_size_ * 2)};
            cv::Size max_size = max_face_size_ < 0 ? cv::Size{} : cv::Size{static_cast<int>(max_face_size_ * 2),
                                                                           static_cast<int>(max_face_size_ * 2)};
            // Params 1.1, 3 and 0 are default values for the method
            haar_cascade_.detectMultiScale(in_mat, faces, 1.1, 3, 0, min_size, max_size);
            if (faces.empty()) {
                log[log::debug] << "No faces found!";
            } else {
                log[log::debug] << "Found " << faces.size() << " faces";
                std::vector<event::pBasicEvent> face_events;

                std::vector<cv::Rect> faces_sorted;
                faces_sorted.reserve(faces.size());
                std::copy_if(faces.cbegin(), faces.cend(), std::back_inserter(faces_sorted),
                             [&](const cv::Rect &f) {
                                 return (f.width / 2) >= min_face_size_ && (f.height / 2) >= min_face_size_
                                        && (max_face_size_ < 0 ||
                                            ((f.width / 2) <= max_face_size_ && (f.height / 2) <= max_face_size_));
                             });
                std::sort(faces_sorted.begin(), faces_sorted.end(),
                          [](const cv::Rect &a, const cv::Rect &b) { return b.width < a.width; });
                for (const auto &x: faces_sorted) {
                    log[log::verbose_debug] << x.width << "x" << x.height << "+" << x.x << "+" << x.y;
                    std::vector<event::pBasicEvent> vec
                            {std::make_shared<event::EventInt>(x.x + x.width / 2, 0, res.width),
                             std::make_shared<event::EventInt>(x.y + x.height / 2, 0, res.height),
                             std::make_shared<event::EventInt>(x.width / 2)};
                    face_events.push_back(std::make_shared<event::EventVector>(vec));
                }
                emit_event(x_event, faces_sorted[0].x + faces_sorted[0].width / 2, 0, res.width);
                emit_event(y_event, faces_sorted[0].y + faces_sorted[0].height / 2, 0, res.height);

                emit_event(height_event, faces_sorted[0].height / 2);
                emit_event(width_event, faces_sorted[0].width / 2);

                auto e = std::make_shared<event::EventVector>(face_events);
                emit_event("faces", e);
            }

            return frame;

        }

        bool OpenCVFaceDetect::set_param(const core::Parameter &param) {
            if (assign_parameters(param)
                    (haar_cascade_file_, "haar_cascade")
                    (min_face_size_, "min_face_size")
                    (max_face_size_, "max_face_size")) {
                return true;
            }
            return core::SpecializedIOFilter<core::RawVideoFrame>::set_param(param);

        }

    } /* namespace opencv_facedetection */
} /* namespace yuri */

