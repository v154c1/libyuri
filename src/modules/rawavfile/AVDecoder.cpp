/*!
 * @file 		AVDecoder.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		4.3.2017
 * @copyright	Institute of Intermedia, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "AVDecoder.h"
#include "yuri/core/Module.h"
#include "yuri/libav/libav.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/compressed_frame_types.h"

namespace yuri {
namespace avdecoder {

IOTHREAD_GENERATOR(AVDecoder)

core::Parameters AVDecoder::configure()
{
    core::Parameters p                                             = base_type::configure();
    p["threads"]["Number of threads. Set to 0 to auto select"]     = 0;
    p["thread_type"]["Type of threaded decoding - slice, frame or any"] = "any";
    return p;
}

AVDecoder::AVDecoder(const log::Log& _log, core::pwThreadBase parent, const core::Parameters& parameters)
    : base_type(_log, parent, 1, 1, "avdecoder"),
      last_format_(0),
      format_(0),
      threads_(0),
      thread_type_(libav::thread_type_t::any),
      ctx_(nullptr, [](AVCodecContext* ctx) { avcodec_free_context(&ctx); }),
      codec_(nullptr)
{
    libav::init_libav();
    IOTHREAD_INIT(parameters)
    set_latency(10_us);
    avframe = av_frame_alloc();
    avpkt_.reset(av_packet_alloc());
}

AVDecoder::~AVDecoder() noexcept
{
//    av_packet_unref(&avpkt_);
    av_frame_free(&avframe);
}

bool AVDecoder::reset_decoder(const core::pCompressedVideoFrame& frame)
{
    // TODO: Delete old pointers

    auto format   = frame->get_format();
    auto avformat = libav::avcodec_from_yuri_format(format);
    if (avformat < 1) {
        log[log::error] << "Unsupported coded";
        return false;
    }
    codec_ = avcodec_find_decoder(avformat);

    if (!codec_)
        return false;
    ctx_.reset(avcodec_alloc_context3(codec_));

    if (!ctx_) {
        return false;
    }

    // Is this really needed?
    if (codec_->capabilities & AV_CODEC_CAP_TRUNCATED) {
        ctx_->flags |= AV_CODEC_FLAG_TRUNCATED;
    }
    if (codec_->capabilities & AV_CODEC_CAP_PARAM_CHANGE) {
        ctx_->flags |= AV_CODEC_CAP_PARAM_CHANGE;
    }
    if (codec_->capabilities & AV_CODEC_FLAG2_CHUNKS)
        ctx_->flags |= AV_CODEC_FLAG2_CHUNKS;
    ctx_->pix_fmt = AV_PIX_FMT_NONE;

    if (codec_->capabilities & AV_CODEC_CAP_SLICE_THREADS) {
        ctx_->thread_type  = libav::libav_thread_type(thread_type_);
        ctx_->thread_count = threads_;
    }
    if (format == core::compressed_frame::avc1) {
        ctx_->codec_tag = ('1' << 24) + ('C' << 16) + ('V' << 8) + 'A';
        libav::set_opt(ctx_->priv_data, "is_avc", 1);
        libav::set_opt(ctx_->priv_data, "nal_length_size", 4);
    }
    if (avcodec_open2(ctx_, codec_, 0) < 0) {
        log[log::error] << "Failed to open codec";
        return false;
    }
    last_format_ = format;
    return true;
}

bool AVDecoder::step()
{
    auto frame = std::dynamic_pointer_cast<core::CompressedVideoFrame>(pop_frame(0));
    if (!frame)
        return true;
    if (frame->get_format() != last_format_) {
        if (!reset_decoder(frame))
            return true;
    }

    avpkt_->data   = &(*frame)[0];
    avpkt_->size   = frame->size();
    avcodec_send_packet(ctx_.get(), avpkt_.get());

//    int got_frame = 0;
    while (true) {
        const auto ret = avcodec_receive_frame(ctx_.get(), avframe);
        if (ret == AVERROR_EOF) {
            // Everything OK, quitting
            break;
        } else if (ret == 0) {
            log[log::verbose_debug] << "Got frame " << ret << "/" << avpkt_->size;
            auto fmt = libav::yuri_pixelformat_from_av(ctx_->pix_fmt);
            if (!fmt) {
                log[log::warning] << "Frame decoded into an unsupported format";
            } else {
                auto out_frame = libav::yuri_frame_from_av(*avframe);
                push_frame(0, std::move(out_frame));
            }
        } else if (ret == AVERROR(EAGAIN)){
            break;
        } else {
                log[log::info] << "Failed to decode frame";
                break;
        }
    }
    avpkt_->size = 0;
    avpkt_->data = nullptr;
    return true;
}

bool AVDecoder::set_param(const core::Parameter& param)
{
    if (assign_parameters(param) //
        (threads_, "threads")    //
            .parsed<std::string>(thread_type_, "thread_type", libav::parse_thread_type)//
        )
        return true;
    return base_type::set_param(param);
}
}
}
