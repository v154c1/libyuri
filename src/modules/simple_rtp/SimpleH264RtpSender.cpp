/*!
 * @file 		SimpleH264RtpSender.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		06.03.2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2017
 * 				Distributed under modified BSD Licence, details in file
 * doc/LICENSE
 *
 */

#include "SimpleH264RtpSender.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/socket/DatagramSocketGenerator.h"
#include "yuri/core/utils/global_time.h"
namespace yuri {
namespace simple_rtp {

IOTHREAD_GENERATOR(SimpleH264RtpSender)

core::Parameters SimpleH264RtpSender::configure()
{
    core::Parameters p = core::IOThread::configure();
    p.set_description("SimpleH264RtpSender");
    p["mtu"]["MTU"]                = 1500;
    p["ssrc"]["SSRC"]              = 0;
    p["address"]["Remote address"] = "127.0.0.1";
    p["socket_type"]               = "yuri_udp";
    p["port"]                      = 57120;
    return p;
}

SimpleH264RtpSender::SimpleH264RtpSender(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : base_type(log_, parent, std::string("simple_rtp")),
      mtu_(1500),
      ssrc_(0x1234),
      sequence_{ 0 },
      address_{ "127.0.0.1" },
      port_{ 0x1256 },
      socket_type_{ "yuri_udp" }
{
    IOTHREAD_INIT(parameters)
}

SimpleH264RtpSender::~SimpleH264RtpSender() noexcept
{
}

void SimpleH264RtpSender::run()
{
    log[log::info] << "Initializing socket of type '" << socket_type_ << "'";
    socket_ = core::DatagramSocketGenerator::get_instance().generate(socket_type_, log, "");
    log[log::info] << "Binding socket";
    if (!socket_->connect(address_, port_)) {
        log[log::fatal] << "Failed to bind socket!";
        request_end(core::yuri_exit_interrupted);
        return;
    }
    log[log::info] << "Socket initialized";
    base_type::run();
}

namespace {

struct data_view {
    uint8_t* ptr;
    size_t   size;
    size_t   remaining;
};

/*!
 * Verifies that data have a valid h264 start prefix
 * @param data Input data
 * @return size of the prefix
 */

size_t is_start_prefix(const data_view& data)
{
    if (data.size < 4)
        return 0;
    if (data.ptr[0] == 0 && data.ptr[1] == 0) {
        if (data.ptr[2] == 1 && ((data.ptr[3] & 0x80) == 0)) {
            return 3;
        }
        if (data.size > 4 && data.ptr[2] == 0 && data.ptr[3] == 1 && ((data.ptr[4] & 0x80) == 0)) {
            return 4;
        }
    }
    return 0;
}

data_view find_nal(data_view data)
{
    auto preffix_len = is_start_prefix(data);
    if (!preffix_len)
        return { nullptr, 0, 0 };
    // We should try to find other NALs here...
    for (auto i = preffix_len; i < data.size; ++i) {
        if (is_start_prefix({ data.ptr + i, data.size - i, data.remaining })) {
            return { data.ptr + preffix_len, i - preffix_len, data.size - i };
        }
    }
    return { data.ptr + preffix_len, data.size - preffix_len, 0 };
}
}

core::pFrame SimpleH264RtpSender::do_special_single_step(core::pCompressedVideoFrame frame)
{
    if (frame->get_format() != core::compressed_frame::h264) {
        log[log::warning] << "Unsupported frame format";
        return {};
    }
    data_view dv        = { &(*frame)[0], frame->size(), 0 };
    auto      d         = find_nal(dv);
    auto      timestamp = 9 * (frame->get_timestamp() - core::utils::get_global_start_time()).value / 100;
    while (d.size > 0) {
        log[log::verbose_debug] << "Found nal (" << static_cast<int>(d.ptr[0] & 0x1F) << ") of size " << d.size << " in data block of " << dv.size << "B";

        if (d.size < (mtu_ + RTPPacket::header_size)) {
            // Packetize as single NAL unit packet
            // TODO: set payload type and timestamp
            RTPPacket packet(d.size, 99, sequence_++, timestamp, ssrc_);
            std::copy(d.ptr, d.ptr + d.size, packet.data_begin());
            packet.set_marker_bit();
            send_rtp_packet(packet);
            log[log::verbose_debug] << "Sent small packet " << sequence_;
        } else {
            // Fragment into multiple packets
            auto    nal_head = d.ptr[0];
            uint8_t fu_head  = (nal_head & 0xE0) | 28;   // Type 28 an copy NRI anf F
            nal_head         = (nal_head & 0x1F) | 0x80; // Set S bit
            size_t offset    = 1;
            for (auto remaining = d.size - 1; remaining > 0;) {
                const auto size = std::min(mtu_ - 2, remaining);
                RTPPacket  packet(size + 2, 99, sequence_++, timestamp, ssrc_);
                auto       data = &(*packet.data_begin());
                data[0]         = fu_head;
                if (remaining == size) {
                    nal_head |= 0x40; // Set E bit
                    packet.set_marker_bit();
                }
                data[1]  = nal_head;
                nal_head = nal_head & 0x1F; // Unset S and R bits
                std::copy(d.ptr + offset, d.ptr + offset + size, packet.data_begin() + 2);
                send_rtp_packet(packet);
                remaining -= size;
                offset += size;
                log[log::verbose_debug] << "Sent FU packet " << sequence_ << " (" << size << ")";
            }
        }
        dv.ptr       = d.ptr + d.size;
        dv.size      = d.remaining;
        dv.remaining = 0;

        d = find_nal(dv);
    }
    return {};
}

bool SimpleH264RtpSender::send_rtp_packet(const RTPPacket& packet)
{
    return socket_->send_datagram(packet.data) > 0;
}

bool SimpleH264RtpSender::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)       //
        (mtu_, "mtu")                  //
        (ssrc_, "ssrc")                //
        (address_, "address")          //
        (port_, "port")                //
        (socket_type_, "socket_type")) //
        return true;
    return base_type::set_param(param);
}

} /* namespace simple_rtp */
} /* namespace yuri */
