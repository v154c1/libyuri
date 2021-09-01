/*!
 * @file 		YuriConvertor.cpp
 * @author 		Zdenek Travnicek
 * @date 		13.8.2010
 * @date		16.2.2013
 * * @date		26.5.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "YuriConvert.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/thread/ConverterRegister.h"
#include "yuri/core/utils/irange.h"
#include <future>
#include <cassert>
#include "convert_10bit_rgb.h"
#include "convert_rgb.h"
#include "convert_yuv.h"
#include "convert_yuv422.h"
#include "convert_yuv_rgb.h"
#include "convert_single.h"

namespace yuri {

    namespace video {

        IOTHREAD_GENERATOR(YuriConvertor)



/* ***************************************************************************
  Adding a new converter:

  For most cases, it should be enough to specialize function template convert_line
  and add ADD_CONVERSION to converters in an appropriate file

  in special cases you may need to specialize get_linesize or allocate_frame
/ *************************************************************************** */


        namespace {
            void insert_partial_map(converter_map &conv, const converter_map &part) {
                conv.insert(part.cbegin(), part.cend());
            }

            converter_map get_all_converters() {
                converter_map conv;
                insert_partial_map(conv, get_converters_single());
                insert_partial_map(conv, get_converters_yuv());
                insert_partial_map(conv, get_converters_yuv422());
                insert_partial_map(conv, get_converters_yuv_rgb());
                insert_partial_map(conv, get_converters_rgb());
                insert_partial_map(conv, get_converters_rgb10bit());
                return conv;
            }

            converter_map all_converters() {
                static converter_map conv = get_all_converters();
                return conv;
            }


            template<class T>
            void register_converters(const T &converter_map) {
                for (const auto &conv: converter_map) {
                    REGISTER_CONVERTER(conv.first.first, conv.first.second, "yuri_convert", conv.second.second)
                }
            }
        }

        MODULE_REGISTRATION_BEGIN("yuri_convert")
            REGISTER_IOTHREAD("yuri_convert", YuriConvertor)
            register_converters(all_converters());
        MODULE_REGISTRATION_END()


        core::Parameters YuriConvertor::configure() {
            core::Parameters p = core::SpecializedIOFilter<core::RawVideoFrame>::configure();
            p["colorimetry"]["Colorimetry to use when converting from RGB (BT709, BT601, BT2020)"] = "BT709";
            p["format"]["Output format"] = std::string("YUV422");
            p["full"]["Assume YUV values in full range"] = true;
            p["threads"]["[EXPERIMENTAL] Number of threads to use. (use 1 to keep old behaviour)"] = 1;
            return p;
        }

        YuriConvertor::YuriConvertor(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters)
                : core::SpecializedIOFilter<core::RawVideoFrame>(log_, parent, "YuriConv"),
                  colorimetry_(YURI_COLORIMETRY_REC709), full_range_(true), threads_(0) {
            IOTHREAD_INIT(parameters)
            converters_ = all_converters();
            log[log::info] << "Initialized " << converters_.size() << " converters";
            for (auto it = converters_.begin(); it != converters_.end(); ++it) {
                const format_pair_t &fp = it->first;
                log[log::debug] << "Converter: " << core::raw_format::get_format_name(fp.first) << " -> "
                                << core::raw_format::get_format_name(fp.second);
            }
        }

        YuriConvertor::~YuriConvertor() noexcept {
        }

        core::pFrame YuriConvertor::do_convert_frame(core::pFrame input_frame, format_t target_format) {
            core::pRawVideoFrame outframe;
            core::pRawVideoFrame frame = std::dynamic_pointer_cast<core::RawVideoFrame>(input_frame);
            if (!frame) return outframe;

            format_t in_fmt = frame->get_format();
            format_pair_t conv_pair = std::make_pair(in_fmt, target_format);
            converter_t converter;

            auto it = converters_.find(conv_pair);
            if (it != converters_.end()) converter = it->second.first;
            if (converter) {
                outframe = converter(frame, *this, threads_);
            } else if (in_fmt == target_format) {
                outframe = frame;
            } else {
                log[log::debug] << "Unknown format combination "
                                << core::raw_format::get_format_name(frame->get_format()) << " -> "
                                << core::raw_format::get_format_name(target_format);
            }

            if (outframe) {
                //@ TODO fix this...
                //outframe->set_info(frame->get_info());
                //if (outframe->get_pts() == 0) outframe->set_time(frame->get_pts(), frame->get_dts(), frame->get_duration());

                // FIXME: This may update too many fields....
                outframe->copy_video_params(*frame);
            }
            return outframe;
        }

        core::pFrame YuriConvertor::do_special_single_step(core::pRawVideoFrame frame) {

            return convert_frame(frame, format_);
        }

        namespace {
            colorimetry_t parse_colorimetry(const std::string &clr) {
                if (iequals(clr, "BT709") || iequals(clr, "REC709") || iequals(clr, "BT.709") ||
                    iequals(clr, "REC.709")) {
                    return YURI_COLORIMETRY_REC709;
                } else if (iequals(clr, "BT601") || iequals(clr, "REC601") || iequals(clr, "BT.601") ||
                           iequals(clr, "REC.601")) {
                    return YURI_COLORIMETRY_REC601;
                } else if (iequals(clr, "BT2020") || iequals(clr, "REC2020") || iequals(clr, "BT.2020") ||
                           iequals(clr, "REC.2020")) {
                    return YURI_COLORIMETRY_REC2020;
                } else {
//		log[log::warning] << "Unrecognized colorimetry type " << clr << ". Falling back to REC.709";
                    return YURI_COLORIMETRY_REC709;
                }
            }
        }

        bool YuriConvertor::set_param(const core::Parameter &p) {
            if (assign_parameters(p)
                    .parsed<std::string>
                            (colorimetry_, "colorimetry", parse_colorimetry)
                    .parsed<std::string>
                            (format_, "format", core::raw_format::parse_format)
                            (full_range_, "full")
                            (threads_, "threads")) {
                if (!format_) format_ = core::raw_format::yuyv422;
                return true;
            }
            return core::SpecializedIOFilter<core::RawVideoFrame>::set_param(p);

        }

    }

}




