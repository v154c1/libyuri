
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
	std::string formats;
	for (const auto& fmt: core::raw_audio_format::formats()) {
		for (const auto& name: fmt.second.short_names) {
			formats += name + ", ";
		}
	}
	p["source"]["Pipewire device to use"]="";
	p["channels"]["Channel to capture"]=2;
	p["sample_rate"]["Sample rate to capture"]=48000;
	p["samples"]["Count of samples captured in one run"]=1024;
	p["format"]["Capture format. Valid values: (" + formats + ")"]="s16";
    return p;
}

PipewireInput::PipewireInput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters)
    : core::IOThread(log_, parent, 0, 1, "pipewire_input"), source_(0), pipewire_ready_(false) {
    IOTHREAD_INIT(parameters)
    pipewire_data_.parent = this;
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

    log[log::info] << "Pipewire stream created successfully, samples: " << pipewire_data_.context.samples;
    connect_pipewire(pipewire_data_.context, default_buffers, SPA_DIRECTION_INPUT, source_ ? source_ : PW_ID_ANY);

    pw_thread_loop_start(pipewire_data_.context.thread_loop);
    pw_thread_loop_unlock(pipewire_data_.context.thread_loop);

    return true;
}

void PipewireInput::destroy() {
    destroy_pipewire(pipewire_data_.context);
    pipewire_ready_ = false;
}

std::vector<core::InputDeviceInfo> PipewireInput::enumerate() {
    return enumerate_pipewire("Audio/Source");
}

void PipewireInput::on_event(uint32_t id, const char *type, const struct spa_dict *props) {
    (void) id; // Unused parameter
    (void) type; // Unused parameter
    (void) props; // Unused parameter
    // Will be moved to common
}

void PipewireInput::on_event_removed(uint32_t id) {
    (void) id; // Unused parameter
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
    auto received_samples = size / get_yuri_format_bytes(pipewire_data_.context.format) / pipewire_data_.context.channels;

    auto frame = core::RawAudioFrame::create_empty(pipewire_data_.context.format, pipewire_data_.context.channels, pipewire_data_.context.sample_rate, received_samples);
    log[log::debug] << "Dequeue buffer of samples: " << received_samples << ", channels: " << pipewire_data_.context.channels << ", sample rate: " << pipewire_data_.context.sample_rate;
    memcpy(frame->data(), data, size);
    push_frame(0, frame);

    pw_stream_queue_buffer(pipewire_data_.context.stream, b);
}

void PipewireInput::run() {
    if (init()) {
        pipewire_ready_ = true;
        log[log::info] << "PipewireInput initialized successfully.";
    } else {
        log[log::error] << "Failed to initialize PipewireInput.";
    }
    while (still_running()) {
        sleep(get_latency());
    }
}

bool PipewireInput::set_param(const core::Parameter &param) {
    log[log::info] << "PipewireInput: Setting parameter: " << param.get_name() << " = " << param.get<std::string>();
    if (assign_parameters(param)
        (source_, "source")
        (pipewire_data_.context.channels, "channels")
        (pipewire_data_.context.sample_rate, "sample_rate")
        (pipewire_data_.context.samples, "samples")
        .parsed<std::string>(pipewire_data_.context.format, "format", core::raw_audio_format::parse_format)) {
        return true;
    }
    return core::IOThread::set_param(param);
}

} // namespace pipewire
} // namespace yuri
