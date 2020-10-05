/*
 * DeckLinkOutput.cpp
 *
 *  Created on: Sep 15, 2011
 *      Author: worker
 */

#include "DeckLinkOutput.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/frame/raw_audio_frame_params.h"

#include <cassert>
namespace yuri {

namespace decklink {

IOTHREAD_GENERATOR(DeckLinkOutput)


core::Parameters DeckLinkOutput::configure()
{
	core::Parameters p = DeckLinkBase::configure();
	p.set_description("Outputs input video to BlackMagic Design device (Eclipse, Intensity, ...)");
	// Using default description from DecklinkBase
//	p["format"]["Output format (1080p25, etc)"]="1080p25";
	p["prebuffer"]["Number of frames to prefill in the output buffer."
	                  "Minimal reasonable value is 2. "
	                  "Setting it to higher values may increase playback fluency, but will increase latency"]=3;
//	p["connection"]["Output connection (HDMI, SDI, SVideo, ...). Please note that enabling one output will also enable other compatible outputs"]=std::string("HDMI");
	p["sync"]["Use synchronous frame display."]=true;
	p["stereo"]["Output stereo image."]=false;
	p["format_detection"]["Try to detect video format."]=1;
	p["audio_channels"]["Number of audio channels to output"]=2;
	return p;
}


DeckLinkOutput::DeckLinkOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters)
		:DeckLinkBase(log_,parent,1,0,"DeckLinkOutput"),output(0),
		output_connection(bmdVideoConnectionHDMI),
		/*act_oframe(0),back_oframe(0),*/frame_number(0),
		buffer_num(5),prebuffer(5),enabled(false),sync(true),stereo_mode(false),stereo_usable(false)
{
	IOTHREAD_INIT(parameters)
	if (stereo_mode) resize(2,0);
	if (audio_enabled) resize(3,0);
	if (!init()) {
		throw exception::InitializationFailed("Failed to initialize DeckLink device");
	}
}

DeckLinkOutput::~DeckLinkOutput() noexcept
{

}

bool DeckLinkOutput::init()
{
	if (!init_decklink()) return false;
	IDeckLinkProfileAttributes *attr;
	device->QueryInterface(IID_IDeckLinkProfileAttributes,reinterpret_cast<void**>(&attr));
	//int64_t video_con;
	//attr->GetInt(BMDDeckLinkVideoOutputConnections,&video_con);
	if (device->QueryInterface(IID_IDeckLinkOutput,reinterpret_cast<void**>(&output))!=S_OK) {
		log[log::fatal] << "Failed to get output interface";
		device->Release();
		device=0;
		return false;
	}

	if (prebuffer >= buffer_num) buffer_num = prebuffer+1;

	return true;
}
void DeckLinkOutput::run()
{
	log[log::debug] << "Starting up";
	if (!verify_display_mode()) {
		log[log::error] << "Failed to verify display mode";
		return;
	}
	if (!start_stream()) {
		log[log::error] << "Failed to start stream";
		return;
	}
	IOThread::run();
	stop_stream();
}


bool DeckLinkOutput::set_param(const core::Parameter &p)
{
//	log[log::info] << "Got param " << p.get_name() << ", value: '" << p.get<std::string>() << "'";
	if (p.get_name() == "format_detection") {
		detect_format=p.get<bool>();
	} else if (p.get_name() == "prebuffer") {
		prebuffer=p.get<size_t>();
	} else if (p.get_name() == "sync") {
		sync=p.get<bool>();
	} else if (p.get_name()=="stereo") {
		stereo_mode=p.get<bool>();
	} return DeckLinkBase::set_param(p);
	return true;
}




bool DeckLinkOutput::verify_display_mode()
{
	assert(output);
	yuri::lock_t l(schedule_mutex);
	BMDDisplayMode dsp_mode;
	bool supported;
	stereo_usable= false;
	if (stereo_mode) {
		if (output->DoesSupportVideoMode(bmdVideoConnectionSDI, mode, pixel_format, bmdNoVideoOutputConversion, bmdVideoOutputDualStream3D, &dsp_mode, &supported)!=S_OK) {
			log[log::warning] << "Selected format does not work with 3D. Re-trying without 3D support";
			if (output->DoesSupportVideoMode(bmdVideoConnectionSDI, mode, pixel_format, bmdNoVideoOutputConversion, bmdVideoOutputFlagDefault, &dsp_mode, &supported)!=S_OK) return false;
		} else {
			stereo_usable = true;
		}

	} else if (output->DoesSupportVideoMode(bmdVideoConnectionSDI, mode, pixel_format, bmdNoVideoOutputConversion, bmdVideoOutputFlagDefault, &dsp_mode, &supported)!=S_OK) return false;
	if (!supported) return false;
	IDeckLinkDisplayMode *dm;
	output->GetDisplayMode(dsp_mode, &dm);
	width = dm->GetWidth();
	height = dm->GetHeight();
	dm->GetFrameRate(&value,&scale);
	const char       *modeName;
	if (dm->GetName(&modeName) == S_OK) {
		log[log::info] << "Selected mode " << modeName;
	} else {
		log[log::warning] << "Failed to get mode name!";
	}
	unsigned linesize;
	switch (pixel_format) {
		case bmdFormat8BitYUV: linesize=width*2;break;
		case bmdFormat8BitARGB: linesize=width*4;break;
		case bmdFormat10BitYUV: linesize=16*(width/6+(width%6?1:0));break;
		default: log[log::error] << "Unsupported pixel format";return false;
	}
	out_frames.clear();
	// Disable warning about unused wariable
	(void)linesize;
	//shared_ptr<IDeckLinkMutableVideoFrame> f;
	for (yuri::size_t i=0;i<buffer_num;++i) {
		std::shared_ptr<DeckLink3DVideoFrame>  f(new DeckLink3DVideoFrame(width,height,pixel_format,bmdFrameFlagDefault));
		if (stereo_mode) {
			std::shared_ptr<DeckLink3DVideoFrame> f2(new DeckLink3DVideoFrame(width,height,pixel_format,bmdFrameFlagDefault));
			//f->set_packing_format(bmdVideo3DPackingFramePacking);
			f->set_packing_format(bmdVideo3DPackingLeftOnly);
			//f->set_packing_format(bmdVideo3DPackingTopAndBottom);
			f->add_right(f2);
		}
		/*if (output->CreateVideoFrame(width,height,linesize,pixel_format,bmdFrameFlagDefault,&f)!=S_OK) {
			log[log::error] << "Failed to allocate video frame" << endl;
			return false;
		}
		shared_ptr<DeckLink3DVideoFrame> pf(f,decklink_deleter);*/
		out_frames.push_back(f);
	}
	return true;
}

void DeckLinkOutput::schedule(bool force)
{
	yuri::lock_t l(schedule_mutex);
	//if (!act_oframe) {
	if (!out_frames.size()) {
		log[log::error] << "No frame to schedule!!";
		return;

	}
	HRESULT res;
	bool act;
	output->IsScheduledPlaybackRunning(&act);
	if (!act && !force) return;
	//if ((res=output->ScheduleVideoFrame(act_oframe,value*frame_number,value,scale))!=S_OK) {
	if ((res=output->ScheduleVideoFrame(do_get_active_buffer().get(),value*frame_number,value,scale))!=S_OK) {
		log[log::error] << "Failed to schedule frame "<<frame_number << "! " << bmerr(res) << " (" << HRESULT_CODE(res) << ")";
	}
	log[log::debug] << "Scheduled frame " << frame_number;
	frame_number++;
}
HRESULT STDMETHODCALLTYPE	DeckLinkOutput::ScheduledFrameCompleted (IDeckLinkVideoFrame* /*completedFrame*/, BMDOutputFrameCompletionResult /*result*/)
{
	schedule(false);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE	DeckLinkOutput::ScheduledPlaybackHasStopped ()
{
	enabled = false;
	return S_OK;
}

bool DeckLinkOutput::start_stream()
{
	assert(output);
	HRESULT res;
	BMDVideoOutputFlags flags = bmdVideoOutputFlagDefault;
	if (stereo_usable) flags|=bmdVideoOutputDualStream3D;

	if ((res=output->EnableVideoOutput(mode,flags))!=S_OK) {
		log[log::error] << "Failed to enable display mode! Error: "<< bmerr(res) << " ("<<HRESULT_CODE(res)<<")";
		return false;
	}
	if (audio_enabled) {
		output->EnableAudioOutput(bmdAudioSampleRate48kHz,bmdAudioSampleType16bitInteger,audio_channels,bmdAudioOutputStreamContinuous);
	} else {
		output->DisableAudioOutput();
	}
	return true;
}
bool DeckLinkOutput::stop_stream()
{
	assert(output);
//	HRESULT res;
	output->DisableAudioOutput();
	output->DisableVideoOutput();
	BMDTimeValue t;
	if (!sync) {
		output->StopScheduledPlayback(0,&t,scale);
		bool r;
		output->IsScheduledPlaybackRunning(&r);
		if (r) {
			log[log::warning] << "Playback still running after being stopped";
		}
	}
	/*if ((res=output->EnableVideoOutput(mode,bmdVideoOutputFlagDefault))!=S_OK) {
		log[log::error] << "Failed to enable display mode! Error: "<< bmerr(res) << " ("<<HRESULT_CODE(res)<<")" << endl;
		return false;
	}*/
	return true;
}
bool DeckLinkOutput::enable_stream()
{
	if (enabled) return true;
	for (size_t i=0; i < prebuffer; ++i) schedule(true);
	output->SetScheduledFrameCompletionCallback(this);
	output->StartScheduledPlayback(0,scale,1.0);

	enabled=true;
	return true;
}

bool DeckLinkOutput::step()
{
	using namespace core::raw_format;
	core::pRawVideoFrame f;
//	if (!in[0]) return true;
	/*bool new_frame = false;
	while (f=in[0]->pop_frame()) {
		frame = f;
		new_frame=true;
		break;
	}
	if (!new_frame) return true;*/
	if (!frame) frame = std::dynamic_pointer_cast<core::RawVideoFrame>(pop_frame(0));
	if (stereo_usable) {
//		if (!in[1]) return true;
		if (!frame2) frame2 = std::dynamic_pointer_cast<core::RawVideoFrame>(pop_frame(1));
	}
	if (!frame || (stereo_usable && !frame2)) return true;
	BMDPixelFormat pfmt = 0;
	switch (frame->get_format()) {
		case uyvy422:pfmt = bmdFormat8BitYUV;break;
		case yuv422_v210:pfmt = bmdFormat10BitYUV;break;
		default: pfmt = 0;break;
	}
	if (!pfmt) {
		log[log::error] << "Unsupported pixel format: "<<get_format_name(frame->get_format());
		frame.reset();
		frame2.reset();
		return true;
	}
	if (stereo_usable) {
		if (frame->get_format() != frame2->get_format()) {
			log[log::error] << "Frames for left and right eye has different pixel formats";
			frame.reset();
			frame2.reset();
			return true;
		}
	}
	bool restart_needed = false;
//	core::pFrameInfo fi = frame->get_info();
	std::string format_name = frame->get_format_name();
	if (detect_format && !format_name.empty()) {
		BMDDisplayMode m = parse_format(format_name);
		if (m==bmdModeUnknown) {
			log[log::warning] << "Format specified in incoming frame is not supported!";
		} else if (m!=mode) {
			pixel_format = pfmt;
			mode = m;
			if (!verify_display_mode()) {
				log[log::error] << "Failed to verify display mode for incoming frame";
				frame.reset();
				frame2.reset();
				return true;
			}
			log[log::info] << "Format changed to " << format_name;
			restart_needed = true;
		}
	}
	if (pfmt!=pixel_format) {
		pixel_format=pfmt;
		if (!verify_display_mode()) {
			log[log::error] << "Failed to verify display mode";
			frame.reset();
			frame2.reset();
			return true;
		}
		restart_needed=true;
	}
	if (restart_needed) {
		stop_stream();
		start_stream();
	}
	std::shared_ptr<DeckLink3DVideoFrame> pf = get_next_buffer();
	fill_frame(frame,pf);
	if (stereo_usable) fill_frame(frame2,pf->get_right());
	if (audio_enabled /*&& in[2]*/) {
		using namespace core::raw_audio_format;
		if (!aframe) aframe=std::dynamic_pointer_cast<core::RawAudioFrame>(pop_frame(2));
		if (aframe) {
			uint32_t writen;
			uvector<int16_t> samples(aframe->get_sample_count()*audio_channels);
			if (aframe->get_format() == signed_24bit_be) {
				const uint8_t *data = aframe->data();
//				size_t zeroes=0;
				const size_t chan = aframe->get_channel_count();
				const size_t copy_chans = std::min(chan,static_cast<size_t>(audio_channels));
				const size_t zero_chans = audio_channels - copy_chans;
				size_t skip =(chan-copy_chans)*3;
				int16_t sample;
				auto sample_iter=samples.begin();
				for (size_t i=0;i<aframe->get_sample_count();++i) {
					for (size_t ch=0;ch<copy_chans;++ch) {
						sample = (data[0]<<16)|(data[1]<<8)|data[2];
						*sample_iter++=sample>>8;
						data+=3;
					}
					for (size_t ch=0;ch<zero_chans;++ch) {
						*sample_iter++=0;
					}
					data+=skip;
				}
//				log[log::info] << "Zeroes " << zeroes << "/" <<audio_frame->get_sample_count()<<endl;
			} else if (aframe->get_format() == signed_16bit) {
				const uint8_t *data = aframe->data();
//				size_t zeroes=0;
				const size_t chan = aframe->get_channel_count();
				const size_t copy_chans = std::min(chan,static_cast<size_t>(audio_channels));
				const size_t zero_chans = audio_channels - copy_chans;
				size_t skip =(chan-copy_chans)*2;
				int16_t sample;
				auto sample_iter=samples.begin();
				for (size_t i=0;i<aframe->get_sample_count();++i) {
					for (size_t ch=0;ch<copy_chans;++ch) {
						sample = (data[1]<<8)|(data[0]<<0);
						*sample_iter++=sample>>1;//>>8;
						data+=2;
					}
					for (size_t ch=0;ch<zero_chans;++ch) {
						*sample_iter++=0;
					}
					data+=skip;
				}
			} else {
				log[log::warning] << "Unsupported input format: " << core::raw_audio_format::get_format_name(aframe->get_format());
			}
			if (samples.size()) {
				output->WriteAudioSamplesSync(&samples[0],aframe->get_sample_count(),&writen);
			}

		} else {
			log[log::warning] << "No audio";
		}
	}
	if (sync) {
		output->DisplayVideoFrameSync(pf.get());
	} else {
		rotate_buffers();
		enable_stream();
	}
	frame.reset();
	frame2.reset();
	aframe.reset();
	return true;
}
std::shared_ptr<DeckLink3DVideoFrame> DeckLinkOutput::get_active_buffer()
{
	yuri::lock_t l(schedule_mutex);
	return do_get_active_buffer();
}
std::shared_ptr<DeckLink3DVideoFrame> DeckLinkOutput::do_get_active_buffer()
{

	assert (out_frames.size());
	return out_frames[0];
}
std::shared_ptr<DeckLink3DVideoFrame> DeckLinkOutput::get_next_buffer()
{
	yuri::lock_t l(schedule_mutex);
	return do_get_next_buffer();
}
std::shared_ptr<DeckLink3DVideoFrame> DeckLinkOutput::do_get_next_buffer()
{
	assert (out_frames.size()>1);
	return out_frames[1];
}
void DeckLinkOutput::rotate_buffers()
{
	yuri::lock_t l(schedule_mutex);
	do_rotate_buffers();
}
void DeckLinkOutput::do_rotate_buffers()
{
	std::shared_ptr<DeckLink3DVideoFrame> pf = out_frames[0];
	out_frames.pop_front();
	out_frames.push_back(pf);
}
bool DeckLinkOutput::fill_frame(core::pRawVideoFrame source, std::shared_ptr<DeckLink3DVideoFrame> output)
{
	assert(source);
	unsigned line_width = 0, copy_width, target_width,copy_lines;
	resolution_t res = source->get_resolution();
//	FormatInfo_t fi = core::BasicPipe::get_format_info(source->get_format());
//	if (fi) {
//		line_width=source->get_width()*(fi->bpp >> 3);
//	}
	line_width = PLANE_DATA(source,0).get_line_size();
	size_t bpp = core::raw_format::get_fmt_bpp(source->get_format(), 0);
	if (!line_width) {
		line_width = res.width * bpp / 8;
	}
//	const auto& fi = core::raw_format::get_format_info(source->get_format());

	target_width=width*(bpp >> 3);
	if (source->get_format() == core::raw_format::yuv422_v210) {
		const size_t w = res.width;
		yuri::size_t in_line_width6 = w/6;// + (w%6?1:0);
		if (w==1280) in_line_width6 = 216;
		yuri::size_t line_width6 = width/6 + (width%6?1:0);
		if (width==1280) line_width6 = 216;
		line_width=in_line_width6*16;
		target_width=line_width6*16;
	}
	if (line_width == 0) {
		return false;
	}

	copy_width=std::min(line_width,target_width);
	copy_lines=std::min(source->get_height(),static_cast<size_t>(height));
	log[log::debug] << "Copying " << copy_lines << " lines of " << copy_width << " bytes (of " << line_width << " total bytes)";
	if (line_width*copy_lines > PLANE_SIZE(source,0)) {
		log[log::warning] << "not enough data to copy!!!";
		return false;
	}
	uint8_t *data = PLANE_RAW_DATA(source,0);
	uint8_t *data2;

	//back_oframe->GetBytes(reinterpret_cast<void**>(&data2));
	output->GetBytes(reinterpret_cast<void**>(&data2));
	assert(data2);
	for (size_t h=0;h<copy_lines;++h) {
		//memcpy(data2,data,copy_width);
		std::copy_n(data,copy_width,data2);
		data+=line_width;
		data2+=target_width;
	}
	return true;
}
}
}
