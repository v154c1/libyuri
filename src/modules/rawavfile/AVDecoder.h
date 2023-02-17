/*!
 * @file 		AVDecoder.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		4.3.2017
 * @copyright	Institute of Intermedia, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef MODULES_RAWAVFILE_AVDECODER_H_
#define MODULES_RAWAVFILE_AVDECODER_H_

#include "avcommon.h"
#include "yuri/core/thread/IOThread.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/utils/managed_resource.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace yuri {
namespace avdecoder {

class AVDecoder : public yuri::core::IOThread {
    using base_type = yuri::core::IOThread;

public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    AVDecoder(const log::Log& _log, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~AVDecoder() noexcept;

    virtual bool set_param(const core::Parameter& param) override;

private:
    virtual bool step() override;

    bool reset_decoder(const core::pCompressedVideoFrame& frame);
    format_t                                      last_format_;
    format_t                                      format_;
    int                                           threads_;
    libav::thread_type_t                          thread_type_;
    core::utils::managed_resource<AVCodecContext> ctx_;
    AVCodec*                                      codec_;
    AVFrame*                                      avframe;
    std::unique_ptr<AVPacket, AVPacketDeleter>    avpkt_;
};
}
}

#endif /* MODULES_RAWAVFILE_AVDECODER_H_ */
