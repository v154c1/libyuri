/*!
 * @file 		SimpleH264RtpReceiver.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		06.03.2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "SimpleH264RtpReceiver.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/socket/DatagramSocketGenerator.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "rtp_packet.h"

namespace yuri {
namespace simple_rtp {

IOTHREAD_GENERATOR(SimpleH264RtpReceiver)

core::Parameters SimpleH264RtpReceiver::configure()
{
    core::Parameters p = core::IOThread::configure();
    p.set_description("SimpleH264RtpReceiver");
    p["address"]["Remote address"] = "127.0.0.1";
    p["socket_type"]               = "yuri_udp";
    p["port"]                      = 57120;
    return p;
}

SimpleH264RtpReceiver::SimpleH264RtpReceiver(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : core::IOThread(log_, parent, 0, 1, std::string("simple_rtp")), sequence_{ 0 }, address_{ "127.0.0.1" }, port_{ 0x1256 }, socket_type_{ "yuri_udp" }
{
    IOTHREAD_INIT(parameters)
}

SimpleH264RtpReceiver::~SimpleH264RtpReceiver() noexcept
{
}

namespace {

void add_data(std::vector<uint8_t>& buffer, size_t read_bytes, RTPPacket& packet)
{
    buffer.reserve(buffer.size() + read_bytes - RTPPacket::header_size - 1);
    buffer.insert(buffer.end(), packet.data_begin() + 2, packet.data_begin() + read_bytes - RTPPacket::header_size);
}

std::array<uint8_t, 4> h264_start_code = { 0, 0, 0, 1 };
}
void SimpleH264RtpReceiver::run()
{
    log[log::info] << "Initializing socket of type '" << socket_type_ << "'";
    socket_ = core::DatagramSocketGenerator::get_instance().generate(socket_type_, log, "");
    log[log::info] << "Binding socket";
    if (!socket_->bind(address_, port_)) {
        log[log::fatal] << "Failed to bind socket!";
        request_end(core::yuri_exit_interrupted);
        return;
    }
    log[log::info] << "Socket initialized";

    std::vector<uint8_t> buffer;

    // Allocate large packet
    RTPPacket    packet(65535, 0, 0, 0, 0);
    resolution_t res{ 0, 0 };
    uint32_t     last_timestamp_ = 0;
    while (still_running()) {
        if (socket_->wait_for_data(get_latency())) {
            log[log::verbose_debug] << "reading data";
            const auto read_bytes = socket_->receive_datagram(&packet.data[0], packet.data.size());
            if (sequence_ != packet.get_sequence()) {
                log[log::warning] << "Missing packet(s)! Expected sequence " << sequence_ << ", got " << packet.get_sequence();
            }
            sequence_ = (packet.get_sequence() + 1) % 65535;
            if (last_timestamp_ != packet.get_timestamp()) {
                // We assume that frames with the same timestamp should be merged together
                if (!buffer.empty()) {
                    auto frame = core::CompressedVideoFrame::create_empty(core::compressed_frame::h264, res, buffer.data(), buffer.size());
                    log[log::verbose_debug] << "Sending (single) frame with " << frame->size();
                    push_frame(0, std::move(frame));
                    buffer.clear();
                }
                last_timestamp_ = packet.get_timestamp();
            }
            if (read_bytes > RTPPacket::header_size) {
                // TODO: verify RTP headers and stuff ...
                auto data        = &(*packet.data_begin());
                auto packet_type = data[0] & 0x1F;
                if (packet_type > 0 && packet_type < 24) {
                    // Single NALU packet

                    buffer.insert(buffer.end(), h264_start_code.begin(), h264_start_code.end());
                    buffer.insert(buffer.end(), data, data + read_bytes - RTPPacket::header_size);
                } else
                    switch (packet_type) {
                    case 28: {
                        // Fragmentation Unit A
                        if (data[1] & 0x80) {
                            // Start of fragmented frame
                            buffer.insert(buffer.end(), h264_start_code.begin(), h264_start_code.end());
                            buffer.push_back((data[1] & 0x1F) | (data[0] & 0xE0));
                        }

                        add_data(buffer, read_bytes, packet);
                        if (data[1] & 0x40) {
                            // End of fragmented frame
                            // No special processing here at the moment
                        }
                    }; break;
                    default:
                        log[log::warning] << "Unsupported packet type";
                    }
            }
        }
    }
}

bool SimpleH264RtpReceiver::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)       //
        (address_, "address")          //
        (port_, "port")                //
        (socket_type_, "socket_type")) //
        return true;

    return core::IOThread::set_param(param);
}

} /* namespace simple_rtp */
} /* namespace yuri */
