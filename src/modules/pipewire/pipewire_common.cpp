/*!
 * @file        pipewire_common.cpp
 * @author      Jiri Melnikov <jiri@melnikoff.org>
 * @date        1.6.2025
 * @copyright
 *              Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include <map>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include "yuri/core/frame/raw_audio_frame_params.h"

namespace {

using namespace yuri::core::raw_audio_format;

std::map<yuri::format_t, spa_audio_format> yuri_to_pipewire_formats = {
    {unsigned_8bit,        SPA_AUDIO_FORMAT_U8    },
    {signed_16bit,         SPA_AUDIO_FORMAT_S16_LE},
    {unsigned_16bit,       SPA_AUDIO_FORMAT_U16_LE},
    {signed_24bit,         SPA_AUDIO_FORMAT_S24_LE},
    {unsigned_24bit,       SPA_AUDIO_FORMAT_U24_LE},
    {signed_32bit,         SPA_AUDIO_FORMAT_S32_LE},
    {unsigned_32bit,       SPA_AUDIO_FORMAT_U32_LE},
    {float_32bit,          SPA_AUDIO_FORMAT_F32_LE},
    {float_64bit,          SPA_AUDIO_FORMAT_F64_LE},
    {signed_16bit_be,      SPA_AUDIO_FORMAT_S16_BE},
    {unsigned_16bit_be,    SPA_AUDIO_FORMAT_U16_BE},
    {signed_24bit_be,      SPA_AUDIO_FORMAT_S24_BE},
    {unsigned_24bit_be,    SPA_AUDIO_FORMAT_U24_BE},
    {signed_32bit_be,      SPA_AUDIO_FORMAT_S32_BE},
    {unsigned_32bit_be,    SPA_AUDIO_FORMAT_U32_BE},
    {float_32bit_be,       SPA_AUDIO_FORMAT_F32_BE},
    {float_64bit_be,       SPA_AUDIO_FORMAT_F64_BE},
    {unsigned_8bit_planar, SPA_AUDIO_FORMAT_U8P   },
    {signed_16bit_planar,  SPA_AUDIO_FORMAT_S16P  },
    {signed_32bit_planar,  SPA_AUDIO_FORMAT_S32P  },
    {float_32bit_planar,   SPA_AUDIO_FORMAT_F32P  },
    {float_64bit_planar,   SPA_AUDIO_FORMAT_F64P  },
};

spa_audio_format get_pulse_format(yuri::format_t fmt) {
    auto it = yuri_to_pipewire_formats.find(fmt);
    if (it == yuri_to_pipewire_formats.end()) return SPA_AUDIO_FORMAT_UNKNOWN;
    return it->second;
}

} // namespace

