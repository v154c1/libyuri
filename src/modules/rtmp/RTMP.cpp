/*!
 * @file 		RTMP.cpp
 * @author 		Jiri Melnikov
 * @date 		13.12.2021
 * @date		13.12.2021
 */

#include "RTMP.h"
#include "yuri/core/Module.h"
#include "yuri/libav/libav.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/utils/global_time.h"

#include <cassert>

namespace yuri {
namespace rtmp {

IOTHREAD_GENERATOR(RTMP)

core::Parameters RTMP::configure() {
	core::Parameters p = IOThread::configure();
	p["address"]["RTMP address"]   = std::string("rtmp://a.rtmp.youtube.com/live2/key");
	p["fps"]["Specify framerate."] = 30;
	p["audio_bitrate"]["Specify audio bitrate."] = 128000;
    p["video_bitrate"]["Specify video bitrate."] = 3584000;
	p["audio"]["Allow audio in stream."] = true;
	return p;
}

namespace {

void add_stream(StreamDescription *output_stream, AVFormatContext *fmt_ctx, AVCodec **codec, enum AVCodecID codec_id) {
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec))
        throw(std::runtime_error("Could not find encoder for codec."));
    output_stream->stream = avformat_new_stream(fmt_ctx, nullptr);
    if (!output_stream->stream)
        throw(std::runtime_error("Could not allocate stream."));
    output_stream->stream->id = fmt_ctx->nb_streams-1;
    AVCodecContext *codec_ctx = avcodec_alloc_context3(*codec);
    if (!codec_ctx)
        throw(std::runtime_error("Could not allocate an encoding context."));
    output_stream->enc = codec_ctx;
    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        codec_ctx->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        codec_ctx->bit_rate    = output_stream->bitrate;
        codec_ctx->sample_rate = output_stream->sample_rate;
        if ((*codec)->supported_samplerates) {
            codec_ctx->sample_rate = (*codec)->supported_samplerates[0];
            for (size_t i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == output_stream->sample_rate)
                    codec_ctx->sample_rate = output_stream->sample_rate;
            }
        }
        codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO; // For streaming we always prefer stereo - mono goes to center speaker and make strange noise on YouTube
        if ((*codec)->channel_layouts) {
            codec_ctx->channel_layout = (*codec)->channel_layouts[0];
            for (size_t i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        codec_ctx->channels = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
        output_stream->stream->time_base = av_make_q(1, codec_ctx->sample_rate);
        break;
    case AVMEDIA_TYPE_VIDEO:
        codec_ctx->codec_id = codec_id;
        codec_ctx->bit_rate = output_stream->bitrate;
        codec_ctx->width    = output_stream->width;
        codec_ctx->height   = output_stream->height;
        output_stream->stream->time_base = av_make_q(1, output_stream->fps);
        codec_ctx->time_base       = output_stream->stream->time_base;
        codec_ctx->gop_size      = 12;
        codec_ctx->pix_fmt       = AV_PIX_FMT_YUV420P;
        if (codec_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            codec_ctx->max_b_frames = 2;
        }
        if (codec_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            codec_ctx->mb_decision = 2;
        }
    break;
    default:
        break;
    }
    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

AVFrame *alloc_video_frame(enum AVPixelFormat pix_fmt, int width, int height) {
    AVFrame *frame;
    int ret;
    frame = av_frame_alloc();
    if (!frame)
        return nullptr;
    frame->format = pix_fmt;
    frame->width  = width;
    frame->height = height;

    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0)
        throw(std::runtime_error("Could not allocate frame data."));
    return frame;
}

AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples) {
    AVFrame *frame = av_frame_alloc();
    int ret;
    if (!frame)
        throw(std::runtime_error("Error allocating an audio frame."));
    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;
    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0)
            throw(std::runtime_error("Error allocating an audio buffer."));
    }
    return frame;
}

int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt) {
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

int write_video_frame(AVFormatContext *fmt_ctx, StreamDescription *output_stream) {
    AVCodecContext *codec_ctx = output_stream->enc;
    int got_packet = 0;
    AVPacket pkt = {};
    av_init_packet(&pkt);

    if (!output_stream->sws_ctx) {
        output_stream->sws_ctx = sws_getContext(output_stream->enc->width, output_stream->enc->height,
                                        output_stream->video_format,
                                        output_stream->enc->width, output_stream->enc->height,
                                        output_stream->enc->pix_fmt,
                                        SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!output_stream->sws_ctx)
            throw(std::runtime_error("Could not initialize the conversion context."));
    }

    av_frame_make_writable(output_stream->frame);
    sws_scale(output_stream->sws_ctx, output_stream->tmp_frame->data, output_stream->tmp_frame->linesize, 0, output_stream->tmp_frame->height, output_stream->frame->data, output_stream->frame->linesize);

    output_stream->frame->pts = output_stream->next_pts++;
    auto ret = avcodec_encode_video2(codec_ctx, &pkt, output_stream->frame, &got_packet);
    if (ret < 0)
        throw(std::runtime_error("Error encoding video frame."));
    if (got_packet) {
        ret = write_frame(fmt_ctx, &codec_ctx->time_base, output_stream->stream, &pkt);
    } else {
        ret = 0;
    }
    if (ret < 0)
        throw(std::runtime_error("Error while writing video frame."));

    return (output_stream->tmp_frame || got_packet) ? 0 : 1;
}

int write_audio_frame(AVFormatContext *fmt_ctx, StreamDescription *output_stream) {
    AVCodecContext *codec_ctx;
    AVPacket pkt = {};
    int got_packet;
    int dst_nb_samples;
    av_init_packet(&pkt);
    codec_ctx = output_stream->enc;
    if (output_stream->tmp_frame) {
        dst_nb_samples = av_rescale_rnd(swr_get_delay(output_stream->swr_ctx, codec_ctx->sample_rate)
                         + output_stream->tmp_frame->nb_samples, codec_ctx->sample_rate, codec_ctx->sample_rate, AV_ROUND_UP);
        auto ret = av_frame_make_writable(output_stream->frame);
        if (ret < 0)
            throw(std::runtime_error("Could not make frame writable."));

        ret = swr_convert(output_stream->swr_ctx,
                          output_stream->frame->data, dst_nb_samples,
                          (const uint8_t **)output_stream->tmp_frame->data, output_stream->tmp_frame->nb_samples);
        if (ret < 0)
            throw(std::runtime_error("Error while converting audio frame."));
        output_stream->tmp_frame = output_stream->frame;
        output_stream->tmp_frame->pts = av_rescale_q(output_stream->samples_count, av_make_q(1, codec_ctx->sample_rate), codec_ctx->time_base);
        output_stream->samples_count += dst_nb_samples;
    }
    auto ret = avcodec_encode_audio2(codec_ctx, &pkt, output_stream->tmp_frame, &got_packet);
    if (ret < 0)
        throw(std::runtime_error("Error encoding audio frame."));

    if (got_packet) {
        ret = write_frame(fmt_ctx, &codec_ctx->time_base, output_stream->stream, &pkt);
        if (ret < 0)
            throw(std::runtime_error("Error while writing audio frame."));
    }
    return (output_stream->tmp_frame || got_packet) ? 0 : 1;
}

void open_video(AVCodec *codec, StreamDescription *output_stream, AVDictionary *opt_arg) {
    int ret;
    AVCodecContext *codec_ctx = output_stream->enc;
    AVDictionary *opt = nullptr;
    av_dict_copy(&opt, opt_arg, 0);

    ret = avcodec_open2(codec_ctx, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0)
        throw(std::runtime_error("Could not open video codec."));

    output_stream->frame = alloc_video_frame(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height);
    if (!output_stream->frame)
        throw(std::runtime_error("Could not allocate video frame."));

    output_stream->tmp_frame = alloc_video_frame(output_stream->video_format, output_stream->width, output_stream->height);
    if (!output_stream->tmp_frame)
        throw(std::runtime_error("Could not allocate temporary frame."));

    ret = avcodec_parameters_from_context(output_stream->stream->codecpar, codec_ctx);
    if (ret < 0)
        throw(std::runtime_error("Could not copy the stream parameters."));
}

void open_audio(AVCodec *codec, StreamDescription *output_stream, AVDictionary *opt_arg) {
    AVCodecContext *codec_ctx = output_stream->enc;
    AVDictionary *opt = nullptr;
    av_dict_copy(&opt, opt_arg, 0);
    auto ret = avcodec_open2(codec_ctx, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0)
        throw(std::runtime_error("Could not open audio codec."));
    int nb_samples;
    if (codec_ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 1024;
    else
        nb_samples = codec_ctx->frame_size;
    
    output_stream->frame     = alloc_audio_frame(codec_ctx->sample_fmt, codec_ctx->channel_layout,
                               codec_ctx->sample_rate, nb_samples);
    output_stream->tmp_frame = alloc_audio_frame(output_stream->audio_format, output_stream->channel_layout,
                               output_stream->sample_rate, nb_samples);

    ret = avcodec_parameters_from_context(output_stream->stream->codecpar, codec_ctx);
    if (ret < 0)
        throw(std::runtime_error("Could not copy the stream parameters."));

    /* create resampler context */
    output_stream->swr_ctx = swr_alloc();
    if (!output_stream->swr_ctx)
        throw(std::runtime_error("Could not allocate resampler context."));
    /* set options */
    av_opt_set_int       (output_stream->swr_ctx, "in_channel_count",   output_stream->channels,     0);
    av_opt_set_int       (output_stream->swr_ctx, "in_sample_rate",     output_stream->sample_rate,  0);
    av_opt_set_sample_fmt(output_stream->swr_ctx, "in_sample_fmt",      output_stream->audio_format, 0);
    av_opt_set_int       (output_stream->swr_ctx, "out_channel_count",  codec_ctx->channels,         0);
    av_opt_set_int       (output_stream->swr_ctx, "out_sample_rate",    codec_ctx->sample_rate,      0);
    av_opt_set_sample_fmt(output_stream->swr_ctx, "out_sample_fmt",     codec_ctx->sample_fmt,       0);
    /* initialize the resampling context */
    if ((ret = swr_init(output_stream->swr_ctx)) < 0)
        throw(std::runtime_error("Failed to initialize the resampling context."));
}

void start_stream(AVFormatContext *fmt_ctx, AVDictionary *opt_arg, std::string address) {
	if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		auto ret = avio_open2(&fmt_ctx->pb, address.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
		if (ret < 0)
            throw(std::runtime_error("Could not connect to the destination."));
	}
	auto ret = avformat_write_header(fmt_ctx, &opt_arg);
    if (ret < 0)
        throw(std::runtime_error("Could not write stream header."));
}

static void close_stream(StreamDescription *output_stream) {
    avcodec_free_context(&output_stream->enc);
    av_frame_free(&output_stream->frame);
    av_frame_free(&output_stream->tmp_frame);
    sws_freeContext(output_stream->sws_ctx);
    swr_free(&output_stream->swr_ctx);
}

}

RTMP::RTMP(const log::Log& _log, core::pwThreadBase parent, const core::Parameters& parameters)
    : base_type(_log, parent, 1, 1, "rtmp_output"),
	av_initialized_(false),
	address_("rtmp://a.rtmp.youtube.com/live2/key"),
	fps_(30),
    audio_bitrate_(128000),
	video_bitrate_(3584000),
	audio_(true),
    yuri_audio_frame_(nullptr),
    yuri_video_frame_(nullptr) {
    IOTHREAD_INIT(parameters)
	if (audio_) resize(2,0);
    set_latency(10_us);
}

RTMP::~RTMP() noexcept {

}

void RTMP::initialize() {
	video_st_ = {};
	audio_st_ = {};
    AVCodec *audio_codec, *video_codec;
    AVDictionary *opt = nullptr;

	auto ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, "flv", nullptr);
	if (ret < 0)
        throw(std::runtime_error("Could not allocate output format context."));

    if (yuri_video_frame_) {
        video_st_.width = yuri_video_frame_->get_width();
        video_st_.height = yuri_video_frame_->get_height();
        video_st_.fps = fps_;
        video_st_.bitrate = video_bitrate_;
        video_st_.video_format = libav::avpixelformat_from_yuri(yuri_video_frame_->get_format());
        add_stream(&video_st_, fmt_ctx_, &video_codec, AV_CODEC_ID_H264);
    }
	if (yuri_audio_frame_) {
        audio_st_.sample_rate = yuri_audio_frame_->get_sampling_frequency();
        audio_st_.bitrate = audio_bitrate_;
        audio_st_.channels = yuri_audio_frame_->get_channel_count();
        audio_st_.channel_layout = (audio_st_.channels == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
        audio_st_.audio_format = libav::avsampleformat_from_yuri(yuri_audio_frame_->get_format());
        add_stream(&audio_st_, fmt_ctx_, &audio_codec, AV_CODEC_ID_AAC);
    }

	if (yuri_video_frame_) open_video(video_codec, &video_st_, opt);
	if (yuri_audio_frame_) open_audio(audio_codec, &audio_st_, opt);

    start_stream(fmt_ctx_, opt, address_);

    av_initialized_ = true;
}

void RTMP::deinitialize() {
    if (yuri_video_frame_) close_stream(&video_st_);
    if (yuri_audio_frame_) close_stream(&audio_st_);
    av_initialized_ = false;
}

bool RTMP::step() {
	auto yuri_video_frame = std::dynamic_pointer_cast<core::RawVideoFrame>(pop_frame(0));
	auto yuri_audio_frame = std::dynamic_pointer_cast<core::RawAudioFrame>(pop_frame(1));

    if (yuri_video_frame) yuri_video_frame_ = yuri_video_frame;
    if (yuri_audio_frame) yuri_audio_frame_ = yuri_audio_frame;

	if (!av_initialized_) {
        try {
            if (audio_ && yuri_audio_frame_ && yuri_video_frame_) {
                initialize();
            } else if (!audio_ && yuri_video_frame_) {
                initialize();
            }
        } catch(const std::exception& e) {
            log[log::error] << "Initialization error: " << e.what();
        }
	}

	if (av_initialized_ && yuri_video_frame) {
        AVFrame *av_pic = video_st_.tmp_frame;
		auto no_planes = yuri_video_frame->get_planes_count();
		auto line_size = PLANE_DATA(yuri_video_frame,0).get_line_size();
		for (yuri::size_t i = 0; i < AV_NUM_DATA_POINTERS; i++) {
			if (no_planes>1) line_size = PLANE_DATA(yuri_video_frame,i).get_line_size();
			if (i >= no_planes) {
				av_pic->data[i]=nullptr;
				av_pic->linesize[i]=0;
			} else {
				av_pic->data[i]=reinterpret_cast<uint8_t*>(PLANE_RAW_DATA(yuri_video_frame,i));
				if (yuri_video_frame->get_height()) {
					av_pic->linesize[i]=line_size;
				} else {
					av_pic->linesize[i]=0;
				}
			}
		}
        try {
            write_video_frame(fmt_ctx_, &video_st_);
        } catch(const std::exception& e) {
            log[log::error] << "Not able to send video frame: " << e.what();
            deinitialize();
        }
    }
	if (av_initialized_ && yuri_audio_frame) {
        AVFrame *frame = audio_st_.tmp_frame;
        uint8_t* dst_data = reinterpret_cast<uint8_t*>(frame->data[0]);
        uint8_t* src_data = reinterpret_cast<uint8_t*>(yuri_audio_frame->data());
        auto max_samples = std::min(frame->nb_samples, static_cast<int>(yuri_audio_frame->get_sample_count()));
        if (frame->nb_samples != static_cast<int>(yuri_audio_frame->get_sample_count()))
            log[log::warning] << "Codec samples are not the same as source samples (" << frame->nb_samples << " != " << yuri_audio_frame->get_sample_count() << ")!";
        std::copy(src_data,src_data+max_samples*(yuri_audio_frame->get_sample_size()/8),dst_data);
        frame->pts = audio_st_.next_pts;
        audio_st_.next_pts  += frame->nb_samples;
        try {
            write_audio_frame(fmt_ctx_, &audio_st_);
        } catch(const std::exception& e) {
            log[log::error] << "Not able to send audio frame: " << e.what();
            deinitialize();
        }
    }

    return true;
}

bool RTMP::set_param(const core::Parameter &parameter) {
    if (assign_parameters(parameter)
    	(address_,       "address")
		(fps_,           "fps")
		(audio_bitrate_, "audio_bitrate")
        (video_bitrate_, "video_bitrate")
		(audio_,         "audio")
        )
        return true;
    return IOThread::set_param(parameter);
}

} /* namespace rtmp */
} /* namespace yuri */

