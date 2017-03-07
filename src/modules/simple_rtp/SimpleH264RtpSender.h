/*!
 * @file 		SimpleH264RtpSender.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		06.03.2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2017
 * 				Distributed under modified BSD Licence, details in file
 * doc/LICENSE
 *
 */

#ifndef SIMPLEH264RTPSENDER_H_
#define SIMPLEH264RTPSENDER_H_

#include "rtp_packet.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/socket/DatagramSocket.h"
#include "yuri/core/thread/SpecializedIOFilter.h"
namespace yuri {
namespace simple_rtp {

class SimpleH264RtpSender : public core::SpecializedIOFilter<core::CompressedVideoFrame> {
    using base_type = core::SpecializedIOFilter<core::CompressedVideoFrame>;

public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    SimpleH264RtpSender(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~SimpleH264RtpSender() noexcept;

private:
    virtual core::pFrame do_special_single_step(core::pCompressedVideoFrame frame) override;
    void         run() override;
    virtual bool set_param(const core::Parameter& param) override;

    bool send_rtp_packet(const RTPPacket& packet);
    size_t                                        mtu_;
    uint32_t                                      ssrc_;
    uint16_t                                      sequence_;
    std::shared_ptr<core::socket::DatagramSocket> socket_;
    std::string                                   address_;
    uint16_t                                      port_;
    std::string                                   socket_type_;
};

} /* namespace simple_rtp */
} /* namespace yuri */
#endif /* SIMPLEH264RTPSENDER_H_ */
