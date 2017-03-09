/*!
 * @file 		SimpleH265RtpReceiver.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		06.03.2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SimpleH265RtpReceiver_H_
#define SimpleH265RtpReceiver_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/core/socket/DatagramSocket.h"

namespace yuri {
namespace simple_rtp {

class SimpleH265RtpReceiver : public core::IOThread {
public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    SimpleH265RtpReceiver(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~SimpleH265RtpReceiver() noexcept;

private:
    virtual void run() override;
    virtual bool set_param(const core::Parameter& param) override;

    uint16_t                                      sequence_;
    std::shared_ptr<core::socket::DatagramSocket> socket_;
    std::string                                   address_;
    uint16_t                                      port_;
    std::string                                   socket_type_;
};

} /* namespace simple_rtp */
} /* namespace yuri */
#endif /* SimpleH265RtpReceiver_H_ */
