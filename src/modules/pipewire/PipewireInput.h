/*!
 * @file        PipewireInput.h
 * @author      Jiri Melnikov <jiri@melnikoff.org>
 * @date        11.6.2025
 */

#ifndef PIPEWIREINPUT_H_
#define PIPEWIREINPUT_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include <pipewire/pipewire.h>
#include <spa/param/audio/raw.h>

namespace yuri {
namespace pipewire {

class PipewireInput;

struct PipewireInputContext {
    PipewireInput *parent;
    struct pw_thread_loop *thread_loop;
    struct pw_loop *loop;
    struct pw_context *context;
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook registry_listener;
    struct pw_stream *stream;
};

class PipewireInput : public core::IOThread {
public:
    IOTHREAD_GENERATOR_DECLARATION
    PipewireInput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
    virtual ~PipewireInput() noexcept;
    static core::Parameters configure();
    void on_event(uint32_t id, const char *type, const struct spa_dict *props);
    void on_event_removed(uint32_t id);
    void on_process();

private:
    virtual void run() override;
    virtual bool set_param(const core::Parameter &param) override;

    bool init_pipewire();
    void destroy_pipewire();

    PipewireInputContext pipewire_data_;
    size_t sink_;
    size_t samples_;
    uint32_t sample_rate_;
    uint32_t channels_;
    format_t format_;
    bool pipewire_ready_;
};

} // namespace pipewire
} // namespace yuri

#endif /* PIPEWIREINPUT_H_ */
