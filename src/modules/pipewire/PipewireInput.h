/*!
 * @file        PipewireInput.h
 * @author      Jiri Melnikov <jiri@melnikoff.org>
 * @date        11.6.2025
 */

#ifndef PIPEWIREINPUT_H_
#define PIPEWIREINPUT_H_

#include "pipewire_common.h"
#include "yuri/core/thread/IOThread.h"
#include "yuri/core/frame/RawAudioFrame.h"

namespace yuri {
namespace pipewire {

class PipewireInput;

struct PipewireInputContext {
    PipewireInput *parent;
    PipewireContext context;
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

    bool init();
    void destroy();

    PipewireInputContext pipewire_data_;
    size_t source_;
    bool pipewire_ready_;
};

} // namespace pipewire
} // namespace yuri

#endif /* PIPEWIREINPUT_H_ */
