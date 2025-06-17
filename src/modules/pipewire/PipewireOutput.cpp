/*!
 * @file 		PipewireOutput.cpp
 * @author 		Jiri Melnikov <jiri@melnikoff.org>
 * @date		1.6.2025
 * @copyright	
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "PipewireOutput.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/frame/raw_audio_frame_params.h"
#include "yuri/core/utils/irange.h"

#include <string.h>
#include <unistd.h>

#include "pipewire_common.cpp"

namespace yuri {
namespace pipewire {

IOTHREAD_GENERATOR(PipewireOutput)

core::Parameters PipewireOutput::configure() {
    core::Parameters p = core::SpecializedIOFilter<core::RawAudioFrame>::configure();
    p.set_description("PipewireOutput");
    p["sink"]["pipewire sink to use"]=0;
    return p;
}

PipewireOutput::PipewireOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::SpecializedIOFilter<core::RawAudioFrame>(log_,parent, std::string("pipewire_output")),event::BasicEventProducer(log),
sink_(0),format_(0),samples_(0),sample_rate_(0),channels_(0),pipewire_ready_(false) {
    IOTHREAD_INIT(parameters)
    pipewire_data_.parent = this;
}

PipewireOutput::~PipewireOutput() noexcept {
    destroy_pipewire();
}

namespace {

std::string print_props(const struct spa_dict *props) {
    std::stringstream ss;
    if (props && props->n_items > 0) {
        for (uint32_t i = 0; i < props->n_items; ++i) {
            const auto& item = props->items[i];
            ss << "  " << item.key << "=" << (item.value ? item.value : "(null)") << std::endl;
        }
    }
    return ss.str();
}

static void on_event_c(void *userdata, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props) {
    (void)permissions; // Unused parameter
    (void)version;     // Unused parameter
    struct yuri::pipewire::PipewireOutputContext *data = static_cast<struct yuri::pipewire::PipewireOutputContext *>(userdata);
    if (data->parent) data->parent->on_event(id, type, props);
}

static void on_event_removed_c(void *userdata, uint32_t id) {
    struct yuri::pipewire::PipewireOutputContext *data = static_cast<struct yuri::pipewire::PipewireOutputContext *>(userdata);
    if (data->parent) data->parent->on_event_removed(id);
}


static void on_process_c(void *userdata) {
    struct yuri::pipewire::PipewireOutputContext *data = static_cast<struct yuri::pipewire::PipewireOutputContext *>(userdata);
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

inline event::pBasicEvent prepare_yuri_event(const uint32_t id, const char *name, const char *desc, const char *nick) {
    std::vector<event::pBasicEvent> vec;
    vec.push_back(std::make_shared<event::EventInt>(id));
    vec.push_back(std::make_shared<event::EventString>(name));
    vec.push_back(std::make_shared<event::EventString>(desc));
    vec.push_back(std::make_shared<event::EventString>(nick));
    return std::make_shared<event::EventVector>(std::move(vec));
}

}

void PipewireOutput::on_event(uint32_t id, const char *type, const struct spa_dict *props) {
    if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
        const char *media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
        if (media_class && strcmp(media_class, "Audio/Sink") == 0) {
            const char *name = spa_dict_lookup(props, "node.name") ? spa_dict_lookup(props, "node.name") : "unknown";
            const char *desc = spa_dict_lookup(props, "node.description") ? spa_dict_lookup(props, "node.description") : "unknown";
            const char *nick = spa_dict_lookup(props, "node.nick") ? spa_dict_lookup(props, "node.nick") : "unknown";
            log[log::info] << "Sink node added: id=" << id << ", name=" << name << ", desc=" << desc << ", nick=" << nick;
            devices_[id] = PipewireDevice {
                .name = name,
                .description = desc,
                .nick = nick
            };
            emit_event("pipewire_sink_added", prepare_yuri_event(id, name, desc, nick));
        } else {
            log[log::debug] << "Other node added: id=" << id << ", type=" << type;
            log[log::debug] << std::endl << print_props(props);
        }
    } else {
        log[log::debug] << "Global object added: id=" << id << ", type=" << type;
        log[log::debug] << std::endl << print_props(props);
    }
}

void PipewireOutput::on_event_removed(uint32_t id) {
    auto it = devices_.find(id);
    if (it != devices_.end()) {
        devices_.erase(it);
        emit_event("pipewire_sink_removed", std::make_shared<event::EventInt>(id));
    }
}

void PipewireOutput::on_process() {
    if (pipewire_data_.frames.empty()) {
        log[log::warning] << "PipewireOutput: no frames to process, skipping";
        return;
    }

    struct pw_buffer *buffer_contatiner;
    if ((buffer_contatiner = pw_stream_dequeue_buffer(pipewire_data_.stream)) == nullptr) {
        log[log::warning] << "PipewireOutput: out of buffers, skipping processing";
        return;
    }

    auto buffer = buffer_contatiner->buffer;
    auto buffer_pointer = static_cast<uint8_t *>(buffer->datas[0].data);
    if (buffer_pointer == nullptr) {
        log[log::warning] << "PipewireOutput: buffer pointer is null, cannot process frame";
        return;
    }

    size_t stride_size = (sample_size_ / 8);
    size_t frames_to_copy = buffer->datas[0].maxsize / stride_size;
    if (buffer_contatiner->requested)
        frames_to_copy = std::min(static_cast<size_t>(buffer_contatiner->requested), frames_to_copy);

    pipewire_data_.frames_mutex.lock();
    auto frame = pipewire_data_.frames.front();
    pipewire_data_.frames.pop();
    pipewire_data_.frames_mutex.unlock();

    auto frame_data = frame->data();
    auto frame_size = frame->get_size();
    size_t bytes_to_copy = std::min(frame_size, frames_to_copy * stride_size);
    memcpy(buffer_pointer, frame_data, bytes_to_copy);

    buffer->datas[0].chunk->offset = 0;
    buffer->datas[0].chunk->stride = stride_size;
    buffer->datas[0].chunk->size = bytes_to_copy;

    pw_stream_queue_buffer(pipewire_data_.stream, buffer_contatiner);
}

core::pFrame PipewireOutput::do_special_single_step(core::pRawAudioFrame frame) {
    if (frame->get_format() != format_) {
        format_ = frame->get_format();
        channels_ = frame->get_channel_count();
        sample_size_ = frame->get_sample_size();
        samples_ = frame->get_size() / (sample_size_ / 8);
        sample_rate_ = frame->get_sampling_frequency();
        log[log::info] << "PipewireOutput: Received new format: " << core::raw_audio_format::get_format_name(format_) << ", sample size: " << sample_size_ << ", samples: " << samples_ << ", channels: " << channels_ << ", rate: " << sample_rate_ << ". Will reinitialize Pipewire.";
        if (pipewire_ready_) {
            destroy_pipewire();
        }
    }
    if (pipewire_ready_ != true) {
        if (init_pipewire()) {
            log[log::info] << "PipewireOutput initialized successfully.";
            pipewire_ready_ = true;
        } else {
            log[log::error] << "Failed to initialize PipewireOutput.";
            return {};
        }
    }

    pipewire_data_.frames_mutex.lock();
    pipewire_data_.frames.push(frame);
    pipewire_data_.frames_mutex.unlock();

    return {};
}

bool PipewireOutput::init_pipewire() {
    std::vector<uint8_t> buffer(samples_);
    struct spa_pod_builder spa_builder = SPA_POD_BUILDER_INIT(buffer.data(), static_cast<uint32_t>(buffer.size()));

    pw_init(nullptr, nullptr);

    pipewire_data_.thread_loop = pw_thread_loop_new("audio-dst", nullptr);
    if (!pipewire_data_.thread_loop) return false;

    pipewire_data_.loop = pw_thread_loop_get_loop(pipewire_data_.thread_loop);
    if (!pipewire_data_.loop) {
        destroy_pipewire();
        return false;
    }

    pipewire_data_.context = pw_context_new(pipewire_data_.loop, nullptr, 0);
    if (!pipewire_data_.context) {
        destroy_pipewire();
        return false;
    }

    pipewire_data_.core = pw_context_connect(pipewire_data_.context, nullptr, 0);
    if (!pipewire_data_.core) {
        destroy_pipewire();
        return false;
    }

    pipewire_data_.registry = pw_core_get_registry(pipewire_data_.core, PW_VERSION_REGISTRY, 0);
    if (!pipewire_data_.registry) {
        destroy_pipewire();
        return false;
    }
    
    pw_registry_add_listener(pipewire_data_.registry, &pipewire_data_.registry_listener, &registry_events, &pipewire_data_);

    pw_thread_loop_lock(pipewire_data_.thread_loop); 
    if (pw_thread_loop_start(pipewire_data_.thread_loop) < 0) {
        destroy_pipewire();
        return false;
    }
 
    auto props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE, "Production",
        nullptr);

    pipewire_data_.stream = pw_stream_new_simple(pipewire_data_.loop, "audio-dst", props, &stream_events, &pipewire_data_);
    if (!pipewire_data_.stream) {
        pw_properties_free(props);
        destroy_pipewire();
        return false;
    }
 
    struct spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(
        .format = get_pulse_format(format_),
        .rate = static_cast<uint32_t>(sample_rate_),
        .channels = static_cast<uint32_t>(channels_));
    const struct spa_pod *params[2];
    params[0] = spa_format_audio_raw_build(&spa_builder, SPA_PARAM_EnumFormat, &info);
    params[1] = (const struct spa_pod *) spa_pod_builder_add_object(&spa_builder,
        SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
        SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(2, 2, 4),
        SPA_PARAM_BUFFERS_size, SPA_POD_Int(samples_ * sizeof(int16_t) * 2),
        SPA_PARAM_BUFFERS_stride, SPA_POD_Int(sizeof(int16_t) * 2));
 
    pw_stream_connect(pipewire_data_.stream,
        PW_DIRECTION_OUTPUT,
        !sink_ ? PW_ID_ANY : sink_,
        static_cast<pw_stream_flags>(
            PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS |
            PW_STREAM_FLAG_RT_PROCESS),
        params, 2);


    pw_thread_loop_start(pipewire_data_.thread_loop);
    pw_thread_loop_unlock(pipewire_data_.thread_loop);

    return true;
}

void PipewireOutput::destroy_pipewire() {
    if (pipewire_data_.thread_loop)
        pw_thread_loop_lock(pipewire_data_.thread_loop);
    if (pipewire_data_.stream)
        pw_stream_destroy(pipewire_data_.stream);
    if (pipewire_data_.core)
        pw_core_disconnect(pipewire_data_.core);
    if (pipewire_data_.context)
        pw_context_destroy(pipewire_data_.context);
    if (pipewire_data_.thread_loop)
        pw_thread_loop_unlock(pipewire_data_.thread_loop);
    if (pipewire_data_.thread_loop)
        pw_thread_loop_destroy(pipewire_data_.thread_loop);
    pw_deinit();
    pipewire_ready_ = false;
}

bool PipewireOutput::set_param(const core::Parameter& param)
{
    if (assign_parameters(param) //
        (sink_, "sink")) {
        return true;
    }
    return core::SpecializedIOFilter<core::RawAudioFrame>::set_param(param);
}

} /* namespace pipewire */
} /* namespace yuri */
