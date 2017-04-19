/*!
 * @file 		h264_helper.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		19. 4. 2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2017
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "h264_helper.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/frame/compressed_frame_types.h"

namespace yuri {
namespace h264 {

void fill_frame_with_start(format_t format, uint8_t* dd, const uint8_t* data, size_t size)
{

    if (format == core::compressed_frame::avc1) {
        dd[0] = (size >> 24) & 0xFF;
        dd[1] = (size >> 16) & 0xFF;
        dd[2] = (size >> 8) & 0xFF;
        dd[3] = (size)&0xFF;
    } else {
        dd[0] = 0;
        dd[1] = 0;
        dd[2] = 0;
        dd[3] = 1;
    }
    std::copy(data, data + size, dd + 4);
}

core::pCompressedVideoFrame prepare_frame_with_start(format_t format, resolution_t res, const uint8_t* data, size_t size)
{
    core::pCompressedVideoFrame f  = core::CompressedVideoFrame::create_empty(format, res, size + 4);
    auto                        dd = &(f->get_data())[0];
    fill_frame_with_start(format, dd, data, size);
    return f;
}

size_t h264_extradata_size(const uint8_t* extradata)
{

    if (!extradata)
        return 0;
    if (extradata[0] != 1) {
        return 0;
    }
    const auto numsps = extradata[5] & 0x1f;
    auto       pos    = 6;
    size_t     size   = 0;
    for (const auto i : irange(numsps)) {
        (void)i;
        const auto spslen = static_cast<int>(extradata[pos]) << 8 | static_cast<int>(extradata[pos + 1]);
        pos += 2 + spslen;
        // + 4 for startcode/avc length
        size += spslen + 4;
    }
    const auto numpps = extradata[pos++];
    for (const auto i : irange(numpps)) {
        (void)i;
        const auto ppslen = static_cast<int>(extradata[pos]) << 8 | static_cast<int>(extradata[pos + 1]);
        pos += 2 + ppslen;
        // + 4 for startcode/avc length
        size += ppslen + 4;
    }
    return size;
}
bool h264_fill_extradata(const uint8_t* extradata, core::pCompressedVideoFrame& f)
{
    if (!extradata)
        return false;
    if (extradata[0] != 1) {
        return false;
    }
    const auto format     = f->get_format();
    const auto numsps     = extradata[5] & 0x1f;
    auto       pos        = 6;
    size_t     frame_pos  = 0;
    uint8_t*   frame_data = &f->get_data()[0];
    for (const auto i : irange(numsps)) {
        (void)i;
        const auto spslen = static_cast<int>(extradata[pos]) << 8 | static_cast<int>(extradata[pos + 1]);
        pos += 2;
        fill_frame_with_start(format, frame_data + frame_pos, extradata + pos, spslen);
        frame_pos += spslen + 4;
        pos += spslen;
    }
    const auto numpps = extradata[pos++];
    for (const auto i : irange(numpps)) {
        (void)i;
        const auto ppslen = static_cast<int>(extradata[pos]) << 8 | static_cast<int>(extradata[pos + 1]);
        pos += 2;
        fill_frame_with_start(format, frame_data + frame_pos, extradata + pos, ppslen);
        frame_pos += ppslen + 4;
        pos += ppslen;
    }
    return true;
}

std::vector<core::pCompressedVideoFrame> get_extradata_frames(const uint8_t* extradata, format_t format, resolution_t res)
{
    std::vector<core::pCompressedVideoFrame> frames;
    if (!extradata)
        return frames;
    if (extradata[0] != 1) {
        return frames;
    }
    const auto numsps = extradata[5] & 0x1f;
    auto       pos    = 6;
    for (const auto i : irange(numsps)) {
        (void)i;
        const auto spslen = static_cast<int>(extradata[pos]) << 8 | static_cast<int>(extradata[pos + 1]);
        pos += 2;
        frames.emplace_back(prepare_frame_with_start(format, res, extradata + pos, spslen));
        pos += spslen;
    }
    const auto numpps = extradata[pos++];
    for (const auto i : irange(numpps)) {
        (void)i;
        const auto ppslen = static_cast<int>(extradata[pos]) << 8 | static_cast<int>(extradata[pos + 1]);
        pos += 2;
        frames.emplace_back(prepare_frame_with_start(format, res, extradata + pos, ppslen));

        pos += ppslen;
    }

    return frames;
}
}
}
