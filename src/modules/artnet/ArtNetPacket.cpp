/*!
 * @file 		ArtNetPacket.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		11. 12. 2014
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "ArtNetPacket.h"
#include <array>

namespace yuri {
namespace artnet {

namespace {
constexpr std::array<uint8_t,header_size> default_artnet_header
	{{'A','r','t','-','N','e','t',0, // Magic
	 0x00, 0x50, 	// Opcode
	 0x00, 14,		// Version
	 0x00,			// Sequence
	 0x00,			// Physical if
	 0x00, 0x00,	// Universe
	 0x00, 0x00		// Length
	}};

constexpr uint16_t sequence_offset = 12;
constexpr uint16_t universe_offset = 14;
constexpr uint16_t length_offset = 16;

void write_into_header_16(std::vector<uint8_t>& header_, uint16_t position, uint16_t value)
{
	header_[position+1]=value&0xFF;
	header_[position]=(value>>8)&0xFF;
}

void write_universe_into_header_16(std::vector<uint8_t>& header_, uint16_t position, uint16_t value)
{
	// Lower 8 bits first
	header_[position]=value&0xFF;
	// Followed by upper 7 bits
	header_[position+1]=(value>>8)&0x7F;
}


}

ArtNetPacket::ArtNetPacket(uint16_t universe):data_(default_artnet_header.begin(), default_artnet_header.end())
{
	write_universe_into_header_16(data_, universe_offset, universe);
}


uint8_t& ArtNetPacket::operator[] (uint16_t index)
{
	const uint16_t array_index = index + header_size;
	if (array_index >= data_.size()) {
		if (index > max_values) {
			throw std::out_of_range("Index out of range");
		}
		data_.resize(array_index+1,0);
		write_into_header_16(data_, length_offset, index);
//		data_[header_size-1]=index&0xFF;
//		data_[header_size-2]=(index>>8)&0xFF;
	}
	return data_[array_index];
}
uint8_t ArtNetPacket::operator[] (uint16_t index) const
{
	const uint16_t array_index = index + header_size;
	if (array_index >= data_.size()) throw std::out_of_range("Index out of range");
	return data_[array_index];
}

bool ArtNetPacket::send(core::socket::pDatagramSocket socket)
{
	if (socket->send_datagram(data_)) {
		data_[sequence_offset] = (data_[sequence_offset]+1)&0xFF;
		return true;
	}
	return false;
}

}
}
