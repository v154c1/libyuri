/*!
 * @file 		RTMP.h
 * @author 		Jiri Melnikov
 * @date 		13.12.2021
 * @date		13.12.2021
 */

#ifndef RTMP_H_
#define RTMP_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/utils/managed_resource.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

namespace yuri {
namespace rtmp {

struct StreamDescription {
    AVStream       *stream;
    AVCodecContext *enc;
    AVFrame        *frame;
    AVFrame        *tmp_frame;
    int64_t        next_pts;
    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
    size_t         bitrate;
    size_t         width;          // Video only
    size_t         height;         // Video only
    int            fps;            // Video only
    AVPixelFormat  video_format;   // Video only
    int            sample_rate;    // Audio only
    int            samples_count;  // Audio only
    size_t         channels;       // Audio only
    uint64_t       channel_layout; // Audio only
    AVSampleFormat audio_format;   // Audio only
};

class RTMP: public yuri::core::IOThread {
    using base_type = yuri::core::IOThread;

public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    RTMP(const log::Log& _log, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~RTMP() noexcept;
    virtual bool set_param(const core::Parameter& param) override;

private:
    virtual bool step() override;
	void initialize();
	void deinitialize();

    bool                av_initialized_;

	std::string         address_;
	double              fps_;
	int                 audio_bitrate_;
    int                 video_bitrate_;
    bool                audio_;

    AVFormatContext*    fmt_ctx_;
    StreamDescription   video_st_;
    StreamDescription   audio_st_;

    std::shared_ptr<yuri::core::RawAudioFrame>  yuri_audio_frame_;
    std::shared_ptr<yuri::core::RawVideoFrame>  yuri_video_frame_;
};

} /* namespace rtmp */
} /* namespace yuri */
#endif /* RTMP_H_ */
