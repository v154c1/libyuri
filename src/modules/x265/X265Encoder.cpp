/*!
 * @file 		X265Encoder.cpp
 * @author 		<Your name>
 * @date		30.03.2014
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#include "X265Encoder.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/compressed_frame_types.h"

namespace yuri {
namespace x265 {


IOTHREAD_GENERATOR(X265Encoder)

MODULE_REGISTRATION_BEGIN("x265")
		REGISTER_IOTHREAD("x265_encoder",X265Encoder)
MODULE_REGISTRATION_END()

core::Parameters X265Encoder::configure()
{
	core::Parameters p = base_type::configure();
	p.set_description("X265Encoder");
	std::string profiles, presets, tunes;
	const char* const * prof = x265_profile_names;
	while (*prof) {
		if (!profiles.empty()) profiles += ", " ;
		profiles += *prof++;
	}
	const char* const * tun = x265_tune_names;
	while (*tun) {
		if (!tunes.empty()) tunes += ", " ;
		tunes += *tun++;
	}
	const char* const * pres = x265_preset_names;
	while (*pres) {
		if (!presets.empty()) presets += ", " ;
		presets += *pres++;
	}

	p["profile"]["X265 profile. Available values: ("+profiles+")"]="main";
	p["preset"]["X265 preset. Available values: ("+presets+")"]="ultrafast";
	p["tune"]["X265 tune. Available values: ("+tunes+")"]="zero-latency";
	return p;
}

namespace {
std::map<format_t, int>supported_formats = {
		{core::raw_format::yuv420p, X265_CSP_I420},
//		{core::raw_format::yuv422p, X265_CSP_I422},
		{core::raw_format::yuv444p, X265_CSP_I444}
};

}

X265Encoder::X265Encoder(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
base_type(log_,parent,std::string("x265")),encoder_(nullptr),
frame_number_(0),preset_("ultrafast"),tune_("zerolatency"),profile_("main")
{
	IOTHREAD_INIT(parameters)
	set_supported_formats(supported_formats);
}


X265Encoder::~X265Encoder() noexcept
{
	if (encoder_) {
		x265_encoder_close(encoder_);
//		x265_picture_clean(&picture_in_);
	}
	if (picture_in_) {
		x265_picture_free(picture_in_);
	}
	if (picture_out_) {
		x265_picture_free(picture_out_);
	}
}

core::pFrame X265Encoder::do_special_single_step(core::pRawVideoFrame frame)
{
	auto it = supported_formats.find(frame->get_format());
	if (it == supported_formats.end()) return {};
	const auto res = frame->get_resolution();
	if (!encoder_) {
		x265_param_default_preset(&params_, preset_.c_str(), tune_.c_str());

		params_.sourceWidth = res.width;
		params_.sourceHeight = res.height;
		params_.fpsNum=25;
		params_.fpsDenom = 1;
		params_.internalCsp = it->second;

		x265_param_apply_profile(&params_, profile_.c_str());

		picture_in_ = x265_picture_alloc();
		x265_picture_init(&params_, picture_in_);
		picture_in_->bitDepth = 8;
		picture_out_ = x265_picture_alloc();
		x265_picture_init(&params_, picture_out_);
		picture_out_->bitDepth = 8;
		encoder_ = x265_encoder_open(&params_);
		unsigned int nheader = 0;
		x265_nal *nals;
		if (x265_encoder_headers(encoder_, &nals, &nheader) < 0) {
			log[log::error] << "Failed to encode headers!";
			return {};
		}
		for (unsigned int i=0;i<nheader;++i) {
			process_nal(nals[i]);
		}
	} else {
		if (res.width != static_cast<dimension_t>(params_.sourceWidth) ||
				res.height != static_cast<dimension_t>(params_.sourceHeight)) {
			log[log::warning] << "Frame size changed, ignoring frame";
			return {};
		}
	}
	picture_in_->colorSpace = it->second;
	picture_in_->pts = frame_number_++;
//	picture_in_.img.i_plane = frame->get_planes_count();
	std::vector<void*> orig_planes(frame->get_planes_count());
	for (size_t i = 0;i < frame->get_planes_count(); ++i) {
		orig_planes[i]=picture_in_->planes[i];
		picture_in_->stride[i]=PLANE_DATA(frame,i).get_line_size();
		picture_in_->planes[i]=PLANE_RAW_DATA(frame,i);
	}
	x265_nal* nals;
	unsigned int nal_count = 0;
	if (x265_encoder_encode(encoder_, &nals, &nal_count, picture_in_, picture_out_)) {
		for (unsigned int i=0;i<nal_count;++i) {
			process_nal(nals[i]);
		}
	}
	for (size_t i = 0; i < frame->get_planes_count(); ++i) {
		picture_in_->planes[i]=orig_planes[i];
	}

	return {};
}

void X265Encoder::process_nal(x265_nal& nal)
{
	auto frame = core::CompressedVideoFrame::create_empty(core::compressed_frame::h265,
			resolution_t{static_cast<dimension_t>(params_.sourceWidth), static_cast<dimension_t>(params_.sourceHeight)},
			nal.payload, nal.sizeBytes);
	push_frame(0, std::move(frame));
}

bool X265Encoder::set_param(const core::Parameter& param)
{
	if (param.get_name() == "preset") {
		preset_ = param.get<std::string>();
	} else if (param.get_name() == "tune") {
		tune_ = param.get<std::string>();
	} else if (param.get_name() == "profile") {
		profile_ = param.get<std::string>();
	} else return base_type::set_param(param);
	return true;
}
} /* namespace x265 */
} /* namespace yuri */
