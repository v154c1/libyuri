/*!
 * @file        pipewire_common.cpp
 * @author      Jiri Melnikov <jiri@melnikoff.org>
 * @date        23.6.2025
 */

#include "pipewire_common.h"
#include <map>

namespace yuri {
namespace pipewire {

using namespace core::raw_audio_format;

std::map<format_t, spa_audio_format> yuri_to_pipewire_formats = {
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

bool init_pipewire(PipewireContext &ctx, const char *name) {
    pw_init(nullptr, nullptr);

    ctx.thread_loop = pw_thread_loop_new(name, nullptr);
    if (!ctx.thread_loop) return false;

    ctx.loop = pw_thread_loop_get_loop(ctx.thread_loop);
    if (!ctx.loop) {
        return false;
    }

    ctx.context = pw_context_new(ctx.loop, nullptr, 0);
    if (!ctx.context) {
        return false;
    }

    ctx.core = pw_context_connect(ctx.context, nullptr, 0);
    if (!ctx.core) {
        return false;
    }

    ctx.registry = pw_core_get_registry(ctx.core, PW_VERSION_REGISTRY, 0);
    if (!ctx.registry) {
        return false;
    }

    return true;
}

void destroy_pipewire(PipewireContext &ctx) {
    if (ctx.thread_loop)
        pw_thread_loop_lock(ctx.thread_loop);
    if (ctx.stream)
        pw_stream_destroy(ctx.stream);
    if (ctx.core)
        pw_core_disconnect(ctx.core);
    if (ctx.context)
        pw_context_destroy(ctx.context);
    if (ctx.thread_loop)
        pw_thread_loop_unlock(ctx.thread_loop);
    if (ctx.thread_loop)
        pw_thread_loop_destroy(ctx.thread_loop);
    pw_deinit();
}

} // namespace pipewire
} // namespace yuri
