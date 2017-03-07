/*!
 * @file 		rtp_packet.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		6. 3. 2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file
 * doc/LICENSE
 *
 */

#ifndef SRC_MODULES_SIMPLE_RTP_RTP_PACKET_H_
#define SRC_MODULES_SIMPLE_RTP_RTP_PACKET_H_

#include <vector>
#include <cstdint>

namespace yuri {
namespace simple_rtp {

struct RTPPacket {
    using size_type                        = std::vector<uint8_t>::size_type;
    static constexpr size_type header_size = 12;

    RTPPacket(size_type size, uint8_t payload_type, uint16_t sequence, uint32_t timestamp, uint32_t ssrc) : data(header_size + size)
    {
        data[0]  = 0x80; // Version 2, other fields in byte 0 are set to 0
        data[1]  = payload_type & 0x7f;
        data[2]  = (sequence >> 8) & 0xFF;
        data[3]  = sequence & 0xFF;
        data[4]  = (timestamp >> 24) & 0xFF;
        data[5]  = (timestamp >> 16) & 0xFF;
        data[6]  = (timestamp >> 8) & 0xFF;
        data[7]  = (timestamp >> 0) & 0xFF;
        data[8]  = (ssrc >> 24) & 0xFF;
        data[9]  = (ssrc >> 16) & 0xFF;
        data[10] = (ssrc >> 8) & 0xFF;
        data[11] = (ssrc >> 0) & 0xFF;
    }

    std::vector<uint8_t>::iterator       data_begin() { return data.begin() + header_size; }
    std::vector<uint8_t>::const_iterator data_end() const { return data.end(); }
    size_type                            data_size() const { return data.size() - header_size; }

    void set_marker_bit(bool status = true) { data[1] = (data[1] & 0x7F) | (status ? 0x80 : 00); }
    bool                     get_marker_bit() const { return (data[1] & 0x7F) == 0x7F; }
    uint16_t                 get_sequence() const { return (static_cast<uint16_t>(data[2]) << 8) + data[3]; }

    uint32_t get_timestamp() const
    {
        return (static_cast<uint32_t>(data[4]) << 24) | (static_cast<uint32_t>(data[5]) << 16) | (static_cast<uint32_t>(data[6]) << 8)
            | (static_cast<uint32_t>(data[7]) << 0);
    }
    std::vector<uint8_t> data;
};
}
}

#endif /* SRC_MODULES_SIMPLE_RTP_RTP_PACKET_H_ */
