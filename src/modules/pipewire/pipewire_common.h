/*!
 * @file        pipewire_common.h
 * @author      Jiri Melnikov <jiri@melnikoff.org>
 * @date        23.6.2025
 */

#ifndef PIPEWIRECOMMON_H_
#define PIPEWIRECOMMON_H_

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/frame/raw_audio_frame_params.h"

namespace yuri {
namespace pipewire {

struct PipewireContext {
    struct pw_thread_loop *thread_loop;
    struct pw_loop *loop;
    struct pw_stream *stream;
    struct pw_context *context;
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook registry_listener;
    format_t format;
    uint32_t channels;
    uint32_t sample_rate;
    uint32_t samples;
};

struct PipewireBuffers {
    uint32_t def;
    uint32_t min;
    uint32_t max;
};

constexpr PipewireBuffers default_buffers = { 4, 2, 8 };

spa_audio_format get_pulse_format(yuri::format_t fmt);
unsigned int get_yuri_format_bytes(format_t fmt);

bool init_pipewire(PipewireContext &ctx, const char *name);
void destroy_pipewire(PipewireContext &ctx);
void connect_pipewire(PipewireContext &ctx, PipewireBuffers buffers, spa_direction direction, uint32_t target_id);

} // namespace pipewire
} // namespace yuri

#endif /* PIPEWIRECOMMON_H_ */