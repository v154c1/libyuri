/*!
 * @file 		AVDecoder.h
 * @author 		Zdenek Travnicek
 * @date 		24.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2010 - 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#include "AVDecoder.h"
#include "yuri/core/Module.h"
namespace yuri
{
namespace video
{

REGISTER("avdecoder",AVDecoder)

core::pBasicIOThread AVDecoder::generate(log::Log &_log, core::pwThreadBase parent, core::Parameters& parameters)
	//throw (Exception)
{
	shared_ptr<AVDecoder> d (new AVDecoder(_log,parent,parameters));
	return d;
}
core::pParameters AVDecoder::configure()
{
	core::pParameters p = BasicIOThread::configure();
	//(*p)["codec"]["Explicitly specified codec"]="";
	/*(*p)["width"]=640;*/
	(*p)["use_timestamps"]["Use timestamps"]=false;
	return p;
}

AVDecoder::AVDecoder(log::Log &_log, core::pwThreadBase parent, core::Parameters &parameters) IO_THREAD_CONSTRUCTOR:
		AVCodecBase(_log,parent,"Decoder"),last_pts(0),first_pts(-1),
		decoding_format(YURI_FMT_NONE),use_timestamps(false),first_out_pts(0),
		first_time(boost::posix_time::not_a_date_time)
{
	IO_THREAD_INIT("avdecoder")
	frame.reset(avcodec_alloc_frame());
	latency=100;
}

AVDecoder::~AVDecoder()
{
}

bool AVDecoder::init_decoder(CodecID codec_id, int width, int height)
{
	this->codec_id=codec_id;
	if (!find_decoder()) {
		log[log::error] << "Failed to find decoder" << std::endl;
		return false;
	}
	assert(c);
	//log[log::info] << c << std::endl;
	log[log::info] << "Selected decoder " << c->long_name << std::endl;
//	const PixelFormat *p = c->pix_fmts;
//	log[log::debug] << "formats" << std::endl;
 	/*if (p) while (*p!=PIX_FMT_NONE) {
		log[log::debug] << "Codec supports format " << avcodec_get_pix_fmt_name(*p++) << std::endl;
	}
	*/if (cc) {
		av_free(cc);
		cc = 0;
	}
	if (!init_codec(AVMEDIA_TYPE_VIDEO, width, height, 0, 0, 0)) return false;

	if (cc->time_base.den) {
		time_step=(float)cc->time_base.num/(float)cc->time_base.den;
	} else {
		time_step = 0.0f;
	}
	return true;

}
bool AVDecoder::init_decoder(AVCodecContext *cc)
{
	log[log::debug] << "init_decoder" << std::endl;
	this->cc=cc;
	this->codec_id=cc->codec_id;
	if (cc->time_base.den) {
		time_step=(float)cc->time_base.num/(float)cc->time_base.den;
	} else {
		time_step = 0.0f;
	}
	log [log::debug] << "time_step: " << time_step << std::endl;
	if (!find_decoder()) return false;
	if (!init_codec(AVMEDIA_TYPE_VIDEO,cc->width,cc->height,0,0,0)) return false;
	return true;
}

bool AVDecoder::decode_frame()
{
	if (!in[0] && !input_frame) return true;
	if (!input_frame) input_frame=in[0]->pop_frame();
	if (!input_frame) return true;
	int got_px=0;
	if (!output_frame) {
		if (!regenerate_contexts(input_frame->get_format(),input_frame->get_width(),input_frame->get_height()))
			return true; // Shouldn't this be fatal?

		assert (c && cc && input_frame);



		while (input_frame && !got_px) {
			AVPacket pkt ;//= convert_to_avframe(input_frame);
			av_init_packet(&pkt);
			if (fabs(time_step)<=0.01f) {
				time_step = static_cast<float>(double(input_frame->get_duration())/1.0e6);
//				log[log::info] << "dur: " << input_frame->get_duration() << ", ts:" << time_step<<std::endl;
			}
			pkt.data 	 = reinterpret_cast<uint8_t*>(PLANE_RAW_DATA(input_frame,0));
			pkt.size 	 = PLANE_SIZE(input_frame,0);
			pkt.pts 	 = static_cast<int64_t>(input_frame->get_pts() / time_step /1e6);
			pkt.dts 	 = static_cast<int64_t>(input_frame->get_dts() / time_step /1e6);
			log[log::verbose_debug] << "input PTS: " << pkt.pts << ", orig: " << input_frame->get_pts() <<", ts: " << time_step<< std::endl;
			pkt.duration = static_cast<int64_t>(input_frame->get_duration() / time_step/1e6);
			//if (!(f=in[0]->pop_frame())) return true;
			avcodec_decode_video2(cc,frame.get(),&got_px,&pkt);
			//frame->pts=f->get_pts();
			if (frame->pts<1) frame->pts=pkt.pts;
			// BUG: Following expressions are meaningless...
			//if (first_pts < 0) first_pts = frame->pts;
			//if (first_pts < 0) first_pts = 0;
			if (frame->pts < 0) {
				frame->pts=last_pts;
			} else last_pts=frame->pts;
			if (!(input_frame = in[0]->pop_frame())) continue;

			if (input_frame->get_format() != decoding_format) {
				input_frame.reset();
				break;
			}
		}
	}
	if (got_px || output_frame) do_output_frame();
	return true;

}

void AVDecoder::do_output_frame()
{
	if (!output_frame) {
		output_frame.reset(new core::BasicFrame());
		size_t plane_size = 0;
		yuri::format_t yf = yuri_pixelformat_from_av(cc->pix_fmt);
		if (yf == YURI_FMT_NONE) {
			log[log::warning] << "Unknown output format " << cc->pix_fmt << std::endl;
			return;
		}
		FormatInfo_t fmt = core::BasicPipe::get_format_info(yf);
		if (!fmt) {
			log[log::warning] << "Unknown format! " << yuri_pixelformat_from_av(cc->pix_fmt) << ", converted from " << cc->pix_fmt << std::endl;
			return;
		} else {
			log[log::verbose_debug] << "Outputting " << fmt->long_name << std::endl;
		}
		if (fmt->planes < 1) {
			log[log::error] << "Wrong info for format " << fmt->long_name << std::endl;
			return;
		}
		for (int i = 0; i < 4; ++i) {
			if (frame->linesize[i] < 1) break;

			// libavcodec can have padding bytes at the end of every line. libyuri doesn't support padding bytes atm, so we have to skip them.
			output_frame->set_planes_count(i+1);
			yuri::size_t line_size = width/fmt->plane_x_subs[i];
			plane_size = line_size*height/fmt->plane_y_subs[i];
			assert(line_size <= static_cast<yuri::size_t>(frame->linesize[i]));
//			shared_array<yuri::ubyte_t> smem = allocate_memory_block(plane_size);
			PLANE_DATA(output_frame,i).resize(plane_size);
			yuri::ubyte_t *src=frame->data[i], *dest=PLANE_RAW_DATA(output_frame,i);
			log[log::verbose_debug] << "Copying plane " << i << ", size: "  << plane_size << std::endl;
			for (yuri::uint_t line=0;line<(height/fmt->plane_y_subs[i]);++line) {
				memcpy(dest,src,line_size);
				dest+=line_size;
				src+=frame->linesize[i];
			}
//			(*output_frame)[i].set(smem,plane_size);
		}
	}
	bool do_out = false;
	yuri::size_t pts 	  = static_cast<yuri::size_t>(1.0e6*time_step*frame->pts);
	yuri::size_t dts 	  = 0;//static_cast<yuri::size_t>(1e6*time_step*frame->dts);
	yuri::size_t duration = static_cast<yuri::size_t>(1.0e6*time_step);

	if (!use_timestamps) {
		do_out = true;
	} else if (first_time==boost::posix_time::not_a_date_time) {
		first_time=boost::posix_time::microsec_clock::local_time();
		first_out_pts = pts;
		do_out = true;
	} else {
		boost::posix_time::ptime t = boost::posix_time::microsec_clock::local_time();
		yuri::size_t pts_diff = pts - first_out_pts;
		boost::posix_time::time_duration delta = t - first_time;
		if (static_cast<yuri::usize_t>(delta.total_microseconds()) > pts_diff) {
//			log[log::info] << "TMS: " << delta.total_microseconds() << ", ptd: " << pts_diff << std::endl;
			do_out=true;
		}
	}
	if (do_out) {
//		log[log::info] << "PTS: " << pts << std::endl;
		push_video_frame(0,output_frame,yuri_pixelformat_from_av(cc->pix_fmt),width,height,pts,dts,duration);
		output_frame.reset();
	}
}

float AVDecoder::get_fps()
{
	return (float)cc->time_base.den/(float)cc->time_base.num;
}

//void AVDecoder::force_synchronous_scaler(int w, int h, PixelFormat fmt)
//{
//	if (!scaler) scaler.reset(new AVScaler(log,get_this_ptr()));
//	scaler->set_output_format(w,h,fmt);
//	add_child(scaler);
//}

bool AVDecoder::step()
{
	if (!decode_frame()) return false;
	return true;
}


bool AVDecoder::regenerate_contexts(long format,yuri::size_t width, size_t height)
{
	if (format == decoding_format && c && cc) return true;
	CodecID cod = CODEC_ID_NONE;
	try {
		cod = avcodec_from_yuri_format(format);
	}
	catch (exception::Exception &e) {
		cod = CODEC_ID_NONE;
	}
	if (cod == CODEC_ID_NONE) {
		log[log::warning] << "Unsupported input format: " << core::BasicPipe::get_format_string(format) <<" ("<<format<<")"<< std::endl;
		return false;
	}
	log[log::info] << "Trying to init decoder @ " << width << "x" << height << ", dv? " << (cod==CODEC_ID_DVVIDEO) << std::endl;
	if (!init_decoder(cod,width,height)) {
		log[log::warning] << "Failed to get decoder for format: " << core::BasicPipe::get_format_string(format) << std::endl;
		return false;
	}
	decoding_format = format;
	return true;

}

bool AVDecoder::set_param(const core::Parameter &param)
{
	if (param.name == "use_timestamps") {
		use_timestamps = param.get<bool>();
	} else
		return BasicIOThread::set_param(param);
	return true;

}
}
}
// End of file


