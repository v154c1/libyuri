/*!
 * @file 		WebImageResource.h
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		07.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#ifndef SRC_MODULES_WEBSERVER_WEBIMAGERESOURCE_H_
#define SRC_MODULES_WEBSERVER_WEBIMAGERESOURCE_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "WebResource.h"
#include <random>

namespace yuri {
namespace webserver {

class WebImageResource : public core::SpecializedIOFilter<core::CompressedVideoFrame>, public WebResource {
    using base_type = core::SpecializedIOFilter<core::CompressedVideoFrame>;

public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    WebImageResource(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~WebImageResource() noexcept;

private:
    virtual void         run() override;
    virtual core::pFrame do_special_single_step(core::pCompressedVideoFrame frame) override;
    virtual bool set_param(const core::Parameter& param) override;
    virtual webserver::response_t do_process_request(const webserver::request_t& request) override;
    std::string                             server_name_;
    std::string                             path_;
    core::pCompressedVideoFrame             last_frame_;
    std::mutex                              frame_lock_;
    std::random_device                      random_device_;
    std::mt19937                            rnd_generator_;
    std::uniform_int_distribution<uint64_t> distribution_;
    uint64_t                                etag_;
};
}
}

#endif /* SRC_MODULES_WEBSERVER_WEBIMAGERESOURCE_H_ */
