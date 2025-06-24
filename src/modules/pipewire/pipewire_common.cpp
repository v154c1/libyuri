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

unsigned int get_yuri_format_bytes(format_t fmt) {
    try {
        const auto& fi = core::raw_audio_format::get_format_info(fmt);
        return fi.bits_per_sample / 8;
    } catch (std::runtime_error&) {
        // This should never happen, but let's return a safe value'
        return 4;
    }
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

void connect_pipewire(PipewireContext &ctx, PipewireBuffers buffers, spa_direction direction, uint32_t target_id) {
    std::vector<uint8_t> buffer(ctx.samples);
    struct spa_pod_builder spa_builder = SPA_POD_BUILDER_INIT(buffer.data(), static_cast<uint32_t>(buffer.size()));

    struct spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(
        .format = get_pulse_format(ctx.format),
        .rate = ctx.sample_rate,
        .channels = ctx.channels);
    for (uint32_t i = 0; i < ctx.channels; ++i) info.position[i] = SPA_AUDIO_CHANNEL_AUX0 + i;

    const struct spa_pod *params[2];
    params[0] = spa_format_audio_raw_build(&spa_builder, SPA_PARAM_EnumFormat, &info);
    params[1] = (const struct spa_pod *) spa_pod_builder_add_object(&spa_builder,
        SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
        SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(buffers.def, buffers.min, buffers.max),
        SPA_PARAM_BUFFERS_size, SPA_POD_Int(ctx.samples * get_yuri_format_bytes(ctx.format) * ctx.channels),
        SPA_PARAM_BUFFERS_stride, SPA_POD_Int(get_yuri_format_bytes(ctx.format) * ctx.channels));

    pw_stream_connect(ctx.stream,
        direction,
        target_id,
        static_cast<pw_stream_flags>(
            PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS |
            PW_STREAM_FLAG_RT_PROCESS),
        params, 2);
}

} // namespace pipewire
} // namespace yuri
