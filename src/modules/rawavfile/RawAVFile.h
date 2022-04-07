/*!
 * @file 		RawAVFile.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		09.02.2012
 *  * @date		02.04.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2012 - 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef AVDEMUXER_H_
#define AVDEMUXER_H_

#include "avcommon.h"
#include "yuri/core/thread/IOFilter.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"
#include "yuri/core/utils/managed_resource.h"
extern "C" {
#include <libavformat/avformat.h>
}

#include <vector>

namespace yuri {
namespace rawavfile {

class RawAVFile : public core::IOThread, public event::BasicEventConsumer, public event::BasicEventProducer {
public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    RawAVFile(const log::Log& _log, core::pwThreadBase parent, const core::Parameters& parameters);
    ~RawAVFile() noexcept override;
    bool set_param(const core::Parameter& param) override;

    struct stream_detail_t;

protected:
    void run() override;
    bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;

    bool open_file(const std::string& filename);
    bool push_ready_frames();
    bool process_file_end();

    bool process_undecoded_frame(index_t idx, const AVPacket& packet);
    bool decode_video_frame(index_t idx, AVPacket& packet, AVFrame* av_frame, bool& keep_packet);
    bool decode_audio_frame(index_t idx, const AVPacket& packet, AVFrame* av_frame, bool& keep_packet);

    bool emit_extradata(index_t idx, format_t format);
    void jump_times(const duration_t& delta);


    virtual bool has_next_filename();
    virtual std::string get_next_filename();
protected:
    bool step() override;

private:
    core::utils::managed_resource<AVFormatContext> fmtctx_;

    std::string filename_;
    std::string next_filename_;

    std::vector<stream_detail_t> video_streams_;
    std::vector<stream_detail_t> audio_streams_;

    format_t                  format_out_;
    format_t                  video_format_out_;
    format_t                  audio_format_out_;
    bool                      decode_;
    double                    fps_;
    std::vector<timestamp_t>  next_times_;
    std::vector<core::pFrame> frames_;
    size_t                    max_video_streams_;
    size_t                    max_audio_streams_;
    int                       threads_;
    libav::thread_type_t      thread_type_;

    int         audio_sample_rate_;
    bool        loop_;
    bool        reset_;
    bool        allow_empty_;
    bool        enable_experimental_;
    bool        ignore_timestamps_;
    int         emit_params_interval_;
    int         last_params_emitted_;
    bool        separate_extra_data_;
    bool        paused_;
    bool        keep_open_;
    bool        black_on_end_;
    timestamp_t pause_start_;
};

} /* namespace video */
} /* namespace yuri */
#endif /* AVDEMUXER_H_ */
