/*!
 * @file 		RawAVFile.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		09.02.2012
 *  * @date		02.04.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2012 - 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "RawAVFile.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/frame/raw_audio_frame_params.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/frame/compressed_frame_params.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/utils/assign_events.h"
#include <cassert>
#include "libavcodec/version.h"
#include "yuri/event/EventHelpers.h"
#include "h264_helper.h"
extern "C" {
#include <libswresample/swresample.h>
}

namespace yuri {
namespace rawavfile {

IOTHREAD_GENERATOR(RawAVFile)

namespace {
const std::string  unknown_format = "Unknown";
const std::string& get_format_name_no_throw(format_t fmt)
{
    try {
        return core::raw_format::get_format_name(fmt);
    } catch (std::exception&) {
    }
    try {
        return core::compressed_frame::get_format_name(fmt);
    } catch (std::exception&) {
    }
    try {
        return core::raw_audio_format::get_format_name(fmt);
    } catch (std::exception&) {
    }
    return unknown_format;
}
}

struct RawAVFile::stream_detail_t {
    explicit stream_detail_t(AVStream* stream = nullptr, format_t fmt_out = 0)
        : stream(stream), ctx(nullptr), codec(nullptr), swr_ctx(nullptr), format(0), format_out(fmt_out), sample_rate(0)
    {
        if (stream && stream->codecpar) {
            const auto& par = *(stream->codecpar);
            codec = avcodec_find_decoder(par.codec_id);
            if (codec == nullptr) {
                throw std::runtime_error("Failed to find decoder");
            }
            ctx = avcodec_alloc_context3(codec);
            if (ctx == nullptr) {
                throw std::runtime_error("Failed to allocate decoder context");
            }
            if (avcodec_parameters_to_context(ctx, &par) < 0) {
                throw std::runtime_error("Failed to setup decoder context");
            }
        }
    }
    ~stream_detail_t()
    {
        if (swr_ctx) {
            swr_close(swr_ctx);
            swr_free(&swr_ctx);
        }
    }
    AVStream*       stream;
    AVCodecContext* ctx;
    AVCodec*        codec;
    SwrContext*     swr_ctx;
    format_t        format;
    format_t        format_out;
    resolution_t    resolution{};
    duration_t      delta;
    int             sample_rate;
};

core::Parameters RawAVFile::configure()
{
    core::Parameters p = IOThread::configure();
    p.set_description("rawavsource reads video files using ffmpeg. It can decode the video or output the encoded frames.");
    p["block"]["Threat output pipes as blocking. Specify as max number of frames in output pipe."] = 0;
    p["filename"]["File to open"]                                                                  = "";
    p["decode"]["Decode the stream and push out raw video"]                                        = true;
    p["format"]["Format to decode to"]                                                             = "YUV422";
    p["audio_format"]["Format to decode audio to"]                                                 = "s16";
    p["fps"]["Override framerate. Specify 0 to use original, or negative value to maximal speed."] = 0;
    p["max_video"]["Maximal number of video streams to process"]                                   = 1;
    p["max_audio"]["Maximal number of audio streams to process"]                                   = 0;
    p["loop"]["Loop the video"]                                                                    = true;
    p["allow_empty"]["Allow empty input file"]                                                     = false;
    p["enable_experimental"]["Enable experimental codecs"]                                         = true;
    p["ignore_timestamps"]["Ignore fps (similar to fps < 0), switchable at runtime"]               = false;
    p["audio_sample_rate"]["Force audio sample rate (0 for original)"]                             = 0;
    p["emit_params_interval"]["Interval to resend SPS and PPS for h264 undecoded stream."
                              "Set to 0 to emit only once at the beginning, "
                              "negative values disables emiting completely."]
        = 1;
    p["separate_extra_data"]["Send extradata for h264 (SPS, PPS) as separate frames instead of prepending them to IDR frames"] = false;
    p["threads"]["Number of threads. Set to 0 to auto select"]                                     = 0;
    p["thread_type"]["Type of threaded decoding - slice, frame or any"]                            = "any";
    return p;
}

// TODO: number of output streams should be -1 and custom connect_out should be implemented.
RawAVFile::RawAVFile(const log::Log& _log, core::pwThreadBase parent, const core::Parameters& parameters)
    : IOThread(_log, std::move(parent), 0, 1024, "RawAVSource"),
      BasicEventConsumer(log),
      BasicEventProducer(log),
      fmtctx_(nullptr, [](AVFormatContext* ctx) { avformat_close_input(&ctx); }),
      video_format_out_(0),
      audio_format_out_(0),
      decode_(true),
      fps_(0.0),
      max_video_streams_(1),
      max_audio_streams_(1),
      threads_(0),
      thread_type_(libav::thread_type_t::any),
      loop_(true),
      reset_(false),
      allow_empty_(false),
      enable_experimental_(true),
      ignore_timestamps_(false),
      emit_params_interval_{ 1 },
      last_params_emitted_{ -1 },
      paused_(false)
{
    IOTHREAD_INIT(parameters)
    set_latency(10_us);
#ifdef BROKEN_FFMPEG
    // We probably using BROKEN fork of ffmpeg (libav) or VERY old ffmpeg.
    if (max_audio_streams_ > 0) {
        log[log::warning] << "Using unsupported version of FFMPEG, probably the FAKE libraries distributed by libav project. Audio support disabled";
        max_audio_streams_ = 0;
    }
#endif
    resize(0, max_video_streams_ + max_audio_streams_);
    libav::init_libav();

    if (filename_.empty()) {
        if (!allow_empty_)
            throw exception::InitializationFailed("No filename specified!");
        log[log::info] << "No filename specified, starting without an active video";
    } else {
        if (!open_file(filename_)) {
            if (!allow_empty_)
                throw exception::InitializationFailed("Failed to open file");
            log[log::warning] << "Failed to open file, but allow_empty was specified, so waiting for new filename";
        }
    }
}

RawAVFile::~RawAVFile() noexcept
{
}

bool RawAVFile::open_file(const std::string& filename)
{
    video_streams_.clear();
    audio_streams_.clear();
    frames_.clear();
    // ffmpeg needs locking of open/close functions...
    auto lock = libav::get_libav_lock();
    if (fmtctx_) {
        avformat_close_input(&fmtctx_.get_ptr_ref());
    }
    fmtctx_.reset();

    avformat_open_input(&fmtctx_.get_ptr_ref(), filename.c_str(), nullptr, nullptr);
    if (!fmtctx_) {
        log[log::error] << "Failed to allocate Format context";
        return false;
    }

    if (avformat_find_stream_info(fmtctx_, nullptr) < 0) {
        log[log::fatal] << "Failed to retrieve stream info!";
        return false;
    }

    for (size_t i = 0; i < fmtctx_->nb_streams; ++i) {
        if (fmtctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            log[log::debug] << "Found video stream with id " << i << ".";
            if (video_streams_.size() < max_video_streams_ && fmtctx_->streams[i]->codecpar) {
                stream_detail_t stream { fmtctx_->streams[i], video_format_out_ };
                    if (enable_experimental_ && stream.codec) {
                        stream.ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
                    }
                video_streams_.emplace_back(std::move(stream));
            } else {
                fmtctx_->streams[i]->discard = AVDISCARD_ALL;
            }
        } else if (fmtctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            log[log::debug] << "Found audio stream with id " << i << ".";
            if (audio_streams_.size() < max_audio_streams_ && fmtctx_->streams[i]->codecpar) {
                stream_detail_t stream { fmtctx_->streams[i]};
                audio_streams_.emplace_back(std::move(stream));
            } else {
                fmtctx_->streams[i]->discard = AVDISCARD_ALL;
            }
        } else {
            fmtctx_->streams[i]->discard = AVDISCARD_ALL;
        }
    }
    if (video_streams_.empty() && audio_streams_.empty()) {
        log[log::error] << "No stream in input file!";
        return false;
    }

    frames_.resize(video_streams_.size());
    for (size_t i = 0; i < video_streams_.size(); ++i) {
        video_streams_[i].format = libav::yuri_format_from_avcodec(video_streams_[i].ctx->codec_id);
        // We have to initialize decoder for h264 even if decode ==false in order to distinguish h264 and avc1 later
        if (!decode_ && video_streams_[i].format != core::compressed_frame::h264) {

            if (video_streams_[i].format == 0) {
                log[log::error] << "Unknown format for video stream " << i;
                return false;
            }
            video_streams_[i].resolution
                = resolution_t{ static_cast<dimension_t>(video_streams_[i].ctx->width), static_cast<dimension_t>(video_streams_[i].ctx->height) };
            log[log::info] << "Found video stream with format " << get_format_name_no_throw(video_streams_[i].format) << " and resolution "
                           << video_streams_[i].ctx->width << "x" << video_streams_[i].ctx->height;

        } else {
            video_streams_[i].codec = avcodec_find_decoder(video_streams_[i].ctx->codec_id);
            if (!video_streams_[i].codec) {
                log[log::error] << "Failed to find decoder for video stream " << i;
                return false;
            }
            if (video_streams_[i].codec->capabilities & AV_CODEC_CAP_TRUNCATED)
                video_streams_[i].ctx->flags |= AV_CODEC_FLAG_TRUNCATED;
            if (video_streams_[i].format_out != 0) {
                video_streams_[i].ctx->pix_fmt = libav::avpixelformat_from_yuri(video_streams_[i].format_out);
            }
            if (video_streams_[i].codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) {
                video_streams_[i].ctx->thread_type  = libav::libav_thread_type(thread_type_);
                video_streams_[i].ctx->thread_count = threads_;
            }

            if (avcodec_open2(video_streams_[i].ctx, video_streams_[i].codec, 0) < 0) {
                log[log::error] << "Failed to open codec for video stream " << i;
                return false;
            }

            video_streams_[i].format_out = libav::yuri_pixelformat_from_av(video_streams_[i].ctx->pix_fmt);
            video_streams_[i].resolution
                = resolution_t{ static_cast<dimension_t>(video_streams_[i].ctx->width), static_cast<dimension_t>(video_streams_[i].ctx->height) };

            log[log::info] << "Found video stream with format " << get_format_name_no_throw(video_streams_[i].format) << " and resolution "
                           << video_streams_[i].resolution << ". Decoding to " << get_format_name_no_throw(video_streams_[i].format_out);
        }
    }
    if (decode_) {
        for (size_t i = 0; i < audio_streams_.size(); ++i) {
            audio_streams_[i].codec = avcodec_find_decoder(audio_streams_[i].ctx->codec_id);
            if (!audio_streams_[i].codec) {
                throw exception::InitializationFailed("Failed to find decoder");
            }
            if (avcodec_open2(audio_streams_[i].ctx, audio_streams_[i].codec, 0) < 0) {
                throw exception::InitializationFailed("Failed to open codec!");
            }
            audio_streams_[i].format      = libav::yuri_format_from_avcodec(audio_streams_[i].ctx->codec_id);
            auto fmt_out                  = libav::yuri_audio_from_av(audio_streams_[i].ctx->sample_fmt);
            audio_streams_[i].sample_rate = audio_sample_rate_ > 0 ? audio_sample_rate_ : audio_streams_[i].ctx->sample_rate;
            if (fmt_out != audio_format_out_ || audio_streams_[i].sample_rate != audio_streams_[i].ctx->sample_rate) {
                auto audio_fmt_libav      = libav::avsampleformat_from_yuri(audio_format_out_);
                audio_streams_[i].swr_ctx = swr_alloc();
                libav::set_opt(audio_streams_[i].swr_ctx, "isf", audio_streams_[i].ctx->sample_fmt, 0);
                libav::set_opt(audio_streams_[i].swr_ctx, "osf", audio_fmt_libav, 0);
                libav::set_opt(audio_streams_[i].swr_ctx, "icl", audio_streams_[i].ctx->channel_layout, 0);
                libav::set_opt(audio_streams_[i].swr_ctx, "ocl", audio_streams_[i].ctx->channel_layout, 0);
                libav::set_opt(audio_streams_[i].swr_ctx, "isr", audio_streams_[i].ctx->sample_rate, 0);
                libav::set_opt(audio_streams_[i].swr_ctx, "osr", audio_streams_[i].sample_rate, 0);
                libav::set_opt(audio_streams_[i].swr_ctx, "ich", audio_streams_[i].ctx->channels, 0);
                libav::set_opt(audio_streams_[i].swr_ctx, "och", audio_streams_[i].ctx->channels, 0);

                swr_init(audio_streams_[i].swr_ctx);
            }
            audio_streams_[i].format_out = audio_format_out_;
            log[log::info] << "Found audio stream, format:" << get_format_name_no_throw(audio_streams_[i].format) << ", decoding to "
                           << get_format_name_no_throw(audio_streams_[i].format_out);
            log[log::debug] << "Orig fmt: " << audio_streams_[i].ctx->sample_fmt;
        }
    }

    for (auto i : irange(video_streams_.size())) {
        if (fps_ > 0)
            video_streams_[i].delta = 1_s / fps_;
        else if (fps_ == 0.0) {
            const auto& den = video_streams_[i].stream->avg_frame_rate.den;
            const auto& num = video_streams_[i].stream->avg_frame_rate.num;
            if (num)
                video_streams_[i].delta = 1_s * den / num;
            else {
                log[log::warning] << "No framerate specified for stream " << i << ", using default 25fps";
                video_streams_[i].delta = 1_s / 25;
            }
            log[log::info] << "Delta " << i << " " << video_streams_[i].delta;
        }
    }

    next_times_.resize(video_streams_.size(), timestamp_t{});
    emit_event("filename", filename_);
    return true;
}

bool RawAVFile::push_ready_frames()
{
    bool ready = false;
    for (auto i : irange(video_streams_.size())) {
        if (frames_[i]) {
            if (fps_ >= 0 && !ignore_timestamps_) {
                timestamp_t curr_time;
                if (curr_time < next_times_[i]) {
                    continue;
                } else {
                    next_times_[i] = next_times_[i] + video_streams_[i].delta;
                }
            }
            frames_[i]->set_timestamp(timestamp_t{});
            push_frame(i, std::move(frames_[i]));

            frames_[i].reset();
            ready = true;
            //				continue;
        } else {
            ready = true;
        }
    }
    if (video_streams_.empty()) {
        ready = true;
    }
    return ready;
}

bool RawAVFile::process_file_end()
{
    emit_event("end", true);
    if (loop_) {
        if (!has_next_filename() && fmtctx_) {
            log[log::debug] << "Seeking to the beginning";
            av_seek_frame(fmtctx_, 0, 0, AVSEEK_FLAG_BACKWARD);
            if (decode_) {
                for (auto& s : video_streams_) {
                    avcodec_flush_buffers(s.ctx);
                }
                for (auto& s : audio_streams_) {
                    avcodec_flush_buffers(s.ctx);
                }
            }
        } else {
            filename_ = get_next_filename();
            log[log::info] << "Opening: " << filename_;
            return open_file(filename_);
        }
        return true;
    }
    log[log::error] << "Failed to read next packet";
    request_end(core::yuri_exit_finished);
    return false;
}

bool RawAVFile::process_undecoded_frame(index_t idx, const AVPacket& packet)
{
    format_t format          = video_streams_[idx].format;
    int      nal_length_size = 4;
    // Our AVC1 supports only 4 byte prefix. If the stream has different prefix length, we need to pad it with zeros)
    // extra_bytes should contain number of padding bytes.
    size_t extra_bytes = 0;
    // libav wrapper can not distinguish between h264 and avc1. So we have to do it manually
    if (format == core::compressed_frame::h264) {
        if (libav::get_opt<bool>(video_streams_[idx].ctx->priv_data, "is_avc")) {
            format          = core::compressed_frame::avc1;
            nal_length_size = libav::get_opt<int>(video_streams_[idx].ctx->priv_data, "nal_length_size");
            if (nal_length_size > 4 || nal_length_size < 0) {
                log[log::warning] << "Received invalid nal_length_size: " << nal_length_size << ", ignoring frame";
                return false;
            }
            extra_bytes = 4 - nal_length_size;
        }
    }
    size_t sps_pps_len = 0;

    if (format == core::compressed_frame::h264 || format == core::compressed_frame::avc1) {
        const auto type = packet.data[nal_length_size] & 0x1fu;
        // We emit extra data only for IDR frames when !separate_extra_data
        const bool frame_usable_for_extradata = separate_extra_data_ || type == 5;

        if (frame_usable_for_extradata && ((last_params_emitted_ < 0) || (emit_params_interval_ > 0 && (++last_params_emitted_ == emit_params_interval_)))) {
            if (separate_extra_data_) {
                // find sps and pps and emit it as two separate frames
                emit_extradata(idx, format);
                last_params_emitted_ = 0;
            } else {
                // Reserve space for extradata
                sps_pps_len          = h264::h264_extradata_size(video_streams_[idx].ctx->extradata);
                last_params_emitted_ = 0;
                log[log::info] << "Prepending extra data!";
            }
        }
    }

    core::pCompressedVideoFrame f;
    if ((extra_bytes + sps_pps_len) == 0) {
        f = core::CompressedVideoFrame::create_empty(format, video_streams_[idx].resolution, packet.data, packet.size);
    } else {
        f = core::CompressedVideoFrame::create_empty(format, video_streams_[idx].resolution, packet.size + extra_bytes + sps_pps_len);
        if (sps_pps_len > 0) {
            h264::h264_fill_extradata(video_streams_[idx].ctx->extradata, f);
        }
        auto data_start = f->get_data().begin() + sps_pps_len;
        if (extra_bytes > 0) {
            std::fill(data_start, data_start + extra_bytes, 0);
        }
        std::copy(packet.data, packet.data + packet.size, data_start + extra_bytes);
    }

    frames_[idx] = f;
    log[log::debug] << "Pushing packet with size: " << f->size();
    duration_t dur = 1_s * packet.duration * video_streams_[idx].stream->avg_frame_rate.den / video_streams_[idx].stream->avg_frame_rate.num;
    if (!dur.value)
        dur = video_streams_[idx].delta;
    f->set_duration(dur);

    log[log::debug] << "Found packet!" /* (pts: " << pts << ", dts: " << dts <<*/ ", dur: " << dur;
    log[log::debug] << "num/den:" << video_streams_[idx].stream->avg_frame_rate.num << "/" << video_streams_[idx].stream->avg_frame_rate.den;
    log[log::debug] << "orig pts: " << packet.pts << ", dts: " << packet.dts << ", dur: " << packet.duration;
    return true;
}

    bool RawAVFile::step() {
        return IOThread::step();
    }

    bool RawAVFile::emit_extradata(index_t idx, format_t format)
{
    const auto d  = video_streams_[idx].ctx->extradata;
    auto       fs = h264::get_extradata_frames(d, format, video_streams_[idx].resolution);
    for (auto& f : fs) {
        push_frame(idx, std::move(f));
    }
    return true;
}

/*
Parameter packet should be const, but the woudn't work with the fake ffmpeg (libav). Oh well...
*/
bool RawAVFile::decode_video_frame(index_t idx, AVPacket& packet, AVFrame* av_frame, bool& keep_packet)
{
    keep_packet      = false;

    if (avcodec_send_packet(video_streams_[idx].ctx, &packet) < 0) {
        log[log::warning] << "Failed to send packet to video decoder";
    }

    auto ret = avcodec_receive_frame(video_streams_[idx].ctx, av_frame);

    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        log[log::warning] << "Failed to receive frame from video decoder";
        return false;
    }
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return false;
    }

    auto f = libav::yuri_frame_from_av(*av_frame);
    if (!f) {
        log[log::warning] << "Failed to convert avframe, probably unsupported pixelformat";
        return false;
    }
    if (format_out_ != f->get_format()) {
        log[log::warning] << "Unexpected frame format! Expected '" << get_format_name_no_throw(format_out_) << "', but got '"
                          << get_format_name_no_throw(f->get_format()) << "'";
        format_out_ = f->get_format();
    }

    frames_[idx] = f;
    return true;
}

bool RawAVFile::decode_audio_frame(index_t idx, const AVPacket& packet, AVFrame* av_frame, bool& keep_packet)
{
#ifdef BROKEN_FFMPEG
    // We are probably using BROKEN port of ffmpeg (libav) or VERY old ffmpeg.
    (void)idx;
    (void)packet;
    (void)av_frame;
    (void)keep_packet;

    return false;
#else
    keep_packet     = false;

    if (avcodec_send_packet(audio_streams_[idx].ctx, &packet) < 0) {
        log[log::warning] << "Failed to send packet to video decoder";
    }

    auto ret = avcodec_receive_frame(audio_streams_[idx].ctx, av_frame);

    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        log[log::warning] << "Failed to receive frame from video decoder";
        return false;
    }
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return false;
    }


    if (!audio_streams_[idx].swr_ctx) {
        try {
            const auto& fi = core::raw_audio_format::get_format_info(audio_format_out_);

            size_t data_size = av_frame->nb_samples * av_frame->channels * fi.bits_per_sample / 8;
            auto f = core::RawAudioFrame::create_empty(audio_streams_[idx].format_out, av_frame->channels,
                                                       av_frame->sample_rate, av_frame->data[0],
                                                       data_size);
            if (!f) {
                log[log::warning] << "Failed to convert avframe, probably unsupported pixelformat";
                return false;
            }
            push_frame(idx + max_video_streams_, std::move(f));
        }
        catch (const std::runtime_error& e) {
            log[log::error] << "Failed to get format info for audio: " << e.what();
            return false;
        }
    } else {
        auto output_sample_count = av_rescale_rnd(av_frame->nb_samples, audio_streams_[idx].sample_rate, audio_streams_[idx].ctx->sample_rate, AV_ROUND_UP);
        auto f = core::RawAudioFrame::create_empty(audio_streams_[idx].format_out, av_frame->channels, audio_streams_[idx].sample_rate,
                                                   output_sample_count);
        auto            out_buffer = f->data();
        const auto** in_buffer  = const_cast<const uint8_t**>(av_frame->data);
        auto            ret        = swr_convert(audio_streams_[idx].swr_ctx, &out_buffer, av_frame->nb_samples, in_buffer, av_frame->nb_samples);
        auto            real_buffer_size
            = av_samples_get_buffer_size(nullptr, av_frame->channels, ret, libav::avsampleformat_from_yuri(audio_streams_[idx].format_out), 1);
        log[log::verbose_debug] << "Received " << av_frame->nb_samples << " samples, expected to convert to " << output_sample_count << ", actually got " << ret
                                << " stored in " << f->size() << " bytes, real size: " << real_buffer_size;
        f->resize(real_buffer_size);
        push_frame(idx + max_video_streams_, std::move(f));
    }
    return true;
#endif
}

namespace {
int find_in_stream_group(int index, const std::vector<RawAVFile::stream_detail_t>& streams)
{
    for (auto i : irange(streams.size())) {
        if (index == streams[i].stream->index) {
            return i;
        }
    }
    return -1;
}
}
void RawAVFile::run()
{
    AVPacket packet;
    av_init_packet(&packet);
    AVPacket empty_packet;
    av_init_packet(&empty_packet);
    empty_packet.data    = nullptr;
    empty_packet.size    = 0;
    bool     keep_packet = false;
    bool     finishing   = false;
    AVFrame* av_frame    = av_frame_alloc();

    next_times_.resize(video_streams_.size(), timestamp_t{});

    while (still_running()) {
        process_events();
        if (!fmtctx_) {
            if (!has_next_filename()) {
                wait_for_events(10_ms);
                continue;
            }
        }

        if (reset_ || !fmtctx_) {
            log[log::info] << "RESET";
            if (!process_file_end()) {
                if (!loop_)
                    break;
            }
            reset_ = false;
            if (!fmtctx_) {
                next_times_.clear();
                continue;
            }
        }

        if (paused_ || !push_ready_frames()) {
            sleep(get_latency());
            continue;
        }

        if (!keep_packet) {
            av_packet_unref(&packet);
            if (av_read_frame(fmtctx_, &packet) < 0) {
                finishing = true;
            }
        }

        if (finishing) {
            bool done = true;
            for (auto i : irange(video_streams_.size())) {
                if (!frames_[i]) {
                    decode_video_frame(i, empty_packet, av_frame, keep_packet);
                }
                if (frames_[i])
                    done = false;
            }
            if (done) {
                finishing = false;
                reset_    = true;
            }
            continue;
        }

        auto idx = find_in_stream_group(packet.stream_index, video_streams_);
        if (idx >= 0) {
            if (!decode_) {
                process_undecoded_frame(idx, packet);
            } else {
                if (!decode_video_frame(idx, packet, av_frame, keep_packet))
                    continue;
            }
        } else {
            idx = find_in_stream_group(packet.stream_index, audio_streams_);
            if (idx < 0) {
                continue;
            }

            if (!decode_audio_frame(idx, packet, av_frame, keep_packet)) {
                continue;
            }
        }
    }
    av_free(av_frame);

    av_packet_unref(&empty_packet);
}

void RawAVFile::jump_times(const duration_t& delta)
{
    for (auto& t : next_times_) {
        t += delta;
    }
}
bool RawAVFile::set_param(const core::Parameter& parameter)
{
    if (assign_parameters(parameter)                                              //
        (filename_, "filename")                                                   //
        (decode_, "decode")                                                       //
            .parsed<std::string>                                                  //
        (video_format_out_, "format", core::raw_format::parse_format)             //
            .parsed<std::string>                                                  //
        (audio_format_out_, "audio_format", core::raw_audio_format::parse_format) //
        (fps_, "fps")                                                             //
        (max_video_streams_, "max_video")                                         //
        (max_audio_streams_, "max_audio")                                         //
        (loop_, "loop")                                                           //
        (allow_empty_, "allow_empty")                                             //
        (enable_experimental_, "enable_experimental")                             //
        (ignore_timestamps_, "ignore_timestamps")                                 //
        (audio_sample_rate_, "audio_sample_rate")                                 //
        (emit_params_interval_, "emit_params_interval")                           //
        (separate_extra_data_, "separate_extra_data")                             //
        (threads_, "threads")                                                     //
        .parsed<std::string>(thread_type_, "thread_type", libav::parse_thread_type)//
        )
        return true;
    return IOThread::set_param(parameter);
}

bool RawAVFile::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
    if (event->get_type() == event::event_type_t::bang_event) {
        if (event_name == "reset") {
            reset_ = true;
            return true;
        }
    }
    if (assign_events(event_name, event) //
        (next_filename_, "filename")     //
        (reset_, "reset")                //
        (ignore_timestamps_, "ignore_timestamps"))
        return true;
    if (event_name == "pause" || event_name == "pause_toggle") {
        bool new_state = paused_;
        if (event_name == "pause") {
            new_state = event::lex_cast_value<bool>(event);
        } else if (event_name == "pause_toggle") {
            new_state = !paused_;
        }

        if (new_state != paused_) {
            paused_ = new_state;
            if (new_state) {
                pause_start_ = timestamp_t{};
            } else {
                auto current_time = timestamp_t{};
                auto delta        = current_time - pause_start_;
                log[log::info] << "Continuing after pause of " << delta;
                jump_times(delta);
            }
        }
    }
    duration_t skip_time;
    if (assign_events(event_name, event)(skip_time, "skip", "skip_time")) {
        jump_times(-skip_time);
    }
    // Compatibility with old name
    if (event_name == "observe_timestamp") {
        ignore_timestamps_ = !event::lex_cast_value<bool>(event);
        return true;
    }

    return false;
}

bool RawAVFile::has_next_filename() {
    return !next_filename_.empty();
}

std::string RawAVFile::get_next_filename() {
    std::string n;
    std::swap (next_filename_, n);
    return n;
}

} /* namespace video */
} /* namespace yuri */
