
#include "pipewire_common.h"
#include "PipewireInput.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include <spa/param/audio/format-utils.h>
#include <spa/param/param.h>
#include <spa/param/format.h>
#include <spa/debug/types.h>
#include <spa/param/buffers.h>
#include <spa/utils/result.h>

#include <unistd.h>

namespace yuri {
namespace pipewire {

IOTHREAD_GENERATOR(PipewireInput)

core::Parameters PipewireInput::configure() {
    core::Parameters p = core::IOThread::configure();
    p.set_description("PipewireInput");
    return p;
}

PipewireInput::PipewireInput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters)
    : core::IOThread(log_, parent, 0, 1, "pipewire_input"), sink_(0), samples_(1024), sample_rate_(48000), channels_(2), format_(core::raw_audio_format::signed_16bit), pipewire_ready_(false) {
    IOTHREAD_INIT(parameters)
    pipewire_data_.parent = this;
    if (init()) {
        pipewire_ready_ = true;
        log[log::info] << "PipewireInput initialized successfully.";
    } else {
        log[log::error] << "Failed to initialize PipewireInput.";
    }
}

PipewireInput::~PipewireInput() noexcept {
    destroy();
}

namespace {

static void on_event_c(void *userdata, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props) {
    (void)permissions; // Unused parameter
    (void)version;     // Unused parameter
    struct yuri::pipewire::PipewireInputContext *data = static_cast<struct yuri::pipewire::PipewireInputContext *>(userdata);
    if (data->parent) data->parent->on_event(id, type, props);
}

static void on_event_removed_c(void *userdata, uint32_t id) {
    struct yuri::pipewire::PipewireInputContext *data = static_cast<struct yuri::pipewire::PipewireInputContext *>(userdata);
    if (data->parent) data->parent->on_event_removed(id);
}

static void on_process_c(void *userdata) {
    struct yuri::pipewire::PipewireInputContext *data = static_cast<struct yuri::pipewire::PipewireInputContext *>(userdata);
    if (data->parent) data->parent->on_process();
}

static const struct pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = on_event_c,
    .global_remove = on_event_removed_c,
};

static const struct pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = nullptr,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = nullptr,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = on_process_c,
    .drained = nullptr,
    .command = nullptr,
    .trigger_done = nullptr
};

}

bool PipewireInput::init() {
    std::vector<uint8_t> buffer(samples_);
    struct spa_pod_builder spa_builder = SPA_POD_BUILDER_INIT(buffer.data(), static_cast<uint32_t>(buffer.size()));

    if (!init_pipewire(pipewire_data_.context, "audio-src")) {
        log[log::error] << "Failed to initialize Pipewire context.";
        destroy();
        return false;
    }

    pw_registry_add_listener(pipewire_data_.context.registry, &pipewire_data_.context.registry_listener, &registry_events, &pipewire_data_);

    pw_thread_loop_lock(pipewire_data_.context.thread_loop); 
    if (pw_thread_loop_start(pipewire_data_.context.thread_loop) < 0) {
        destroy();
        return false;
    }

    auto props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Production",
        nullptr);

    pipewire_data_.context.stream = pw_stream_new_simple(pipewire_data_.context.loop, "audio-src", props, &stream_events, &pipewire_data_);
    if (!pipewire_data_.context.stream) {
        pw_properties_free(props);
        destroy();
        return false;
    }

    struct spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(
        .format = get_pulse_format(format_),
        .rate = static_cast<uint32_t>(sample_rate_),
        .channels = static_cast<uint32_t>(channels_));
    // for (uint32_t i = 0; i < channels_; ++i) info.position[i] = SPA_AUDIO_CHANNEL_MONO;

    const struct spa_pod *params[2];
    params[0] = spa_format_audio_raw_build(&spa_builder, SPA_PARAM_EnumFormat, &info);
    params[1] = (const struct spa_pod *) spa_pod_builder_add_object(&spa_builder,
        SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
        SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(2, 2, 4),
        SPA_PARAM_BUFFERS_size, SPA_POD_Int(samples_ * get_yuri_format_bytes(format_) * 2),
        SPA_PARAM_BUFFERS_stride, SPA_POD_Int(get_yuri_format_bytes(format_) * 2));

    pw_stream_connect(pipewire_data_.context.stream,
        PW_DIRECTION_INPUT,
        !sink_ ? PW_ID_ANY : sink_,
        static_cast<pw_stream_flags>(
            PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS |
            PW_STREAM_FLAG_RT_PROCESS),
        params, 2);

    pw_thread_loop_start(pipewire_data_.context.thread_loop);
    pw_thread_loop_unlock(pipewire_data_.context.thread_loop);

    return true;
}

void PipewireInput::destroy() {
    destroy_pipewire(pipewire_data_.context);
    pipewire_ready_ = false;
}

void PipewireInput::on_event(uint32_t id, const char *type, const struct spa_dict *props) {
    // Will be moved to common
}

void PipewireInput::on_event_removed(uint32_t id) {
    // Will be moved to common
}

void PipewireInput::on_process() {
    if (!pipewire_ready_) return;

    struct pw_buffer *b;
    struct spa_buffer *buf;
    if ((b = pw_stream_dequeue_buffer(pipewire_data_.context.stream)) == nullptr) return;
    buf = b->buffer;
    if (!buf->datas[0].data || buf->datas[0].chunk->size == 0) {
        pw_stream_queue_buffer(pipewire_data_.context.stream, b);
        return;
    }

    auto *data = static_cast<uint8_t *>(buf->datas[0].data);
    auto size = buf->datas[0].chunk->size;
    auto samples = size / get_yuri_format_bytes(format_) / channels_;

    auto frame = core::RawAudioFrame::create_empty(format_, channels_, sample_rate_, samples);
    log[log::debug] << "Dequeue buffer of samples: " << samples << ", channels: " << channels_ << ", sample rate: " << sample_rate_;
    memcpy(frame->data(), data, size);
    push_frame(0, frame);

    pw_stream_queue_buffer(pipewire_data_.context.stream, b);
}

void PipewireInput::run() {
    while (still_running()) {
        sleep(get_latency());
    }
}

bool PipewireInput::set_param(const core::Parameter &param) {
    if (assign_parameters(param) //
        (samples_, "samples")) {
        return true;
    }
    return core::IOThread::set_param(param);
}

} // namespace pipewire
} // namespace yuri
