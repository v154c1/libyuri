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

void on_event(void *userdata, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props) {
    (void)permissions; // Unused parameter
    (void)version;     // Unused parameter
    auto tmp_devices = static_cast<PipewireDevicesEnum *>(userdata);
    if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
        const char *media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
        if (media_class && strcmp(media_class, tmp_devices->filter) == 0) {
            const char *name = spa_dict_lookup(props, "node.name") ? spa_dict_lookup(props, "node.name") : "unknown";
            const char *desc = spa_dict_lookup(props, "node.description") ? spa_dict_lookup(props, "node.description") : "unknown";
            const char *nick = spa_dict_lookup(props, "node.nick") ? spa_dict_lookup(props, "node.nick") : "unknown";
            tmp_devices->devices.insert({id, PipewireDevice {
                .name = name,
                .description = desc,
                .nick = nick
            }});
        }
    }
}

static const struct pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = on_event,
};

std::vector<core::InputDeviceInfo> enumerate_pipewire(const char *filter) {
    std::vector<core::InputDeviceInfo> devices;
	std::vector<std::string> main_param_order = {"index","name","description","nick"};

    PipewireContext context;
    if (!init_pipewire(context, "pipewire-enum")) return devices;

    PipewireDevicesEnum tmp_devices;
    tmp_devices.filter = filter; // Filter for audio sinks

    pw_registry_add_listener(context.registry, &context.registry_listener,
        &registry_events,
        &tmp_devices);

    pw_thread_loop_lock(context.thread_loop);
    pw_thread_loop_start(context.thread_loop);
    pw_thread_loop_unlock(context.thread_loop);

    yuri::core::ThreadBase::sleep(enumerate_timeout);

    pw_thread_loop_stop(context.thread_loop);
    destroy_pipewire(context);

    for (const auto& [id, dev] : tmp_devices.devices) {
        core::InputDeviceInfo device;
        device.main_param_order = main_param_order;
        device.device_name = dev.name;
        core::InputDeviceConfig cfg_base;
        cfg_base.params["index"]=std::to_string(id);
		cfg_base.params["name"]=dev.name;
        cfg_base.params["description"]=dev.description;
        cfg_base.params["nick"]=dev.nick;
        device.configurations.push_back(std::move(cfg_base));
		devices.push_back(std::move(device));
    }

    return devices;
}


} // namespace pipewire
} // namespace yuri
