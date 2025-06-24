/*!
 * @file 		PipewireOutput.h
 * @author 		Jiri Melnikov <jiri@melnikoff.org>
 * @date 		1.6.2025
 * @copyright	
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef PIPEWIREOUTPUT_H_
#define PIPEWIREOUTPUT_H_

#include "pipewire_common.h"
#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/event/BasicEventProducer.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include <spa/param/audio/format-utils.h>

#include <map>
#include <math.h>
#include <queue>
#include <mutex>

namespace yuri {
namespace pipewire {

class PipewireOutput;

struct PipewireOutputContext {
    PipewireOutput *parent;
    PipewireContext context;
    std::queue<core::pRawAudioFrame> frames;
    std::mutex frames_mutex;
};

struct PipewireDevice {
    std::string name;
    std::string description;
    std::string nick;
};

class PipewireOutput: public core::SpecializedIOFilter<core::RawAudioFrame>, public event::BasicEventProducer
{
public:
    IOTHREAD_GENERATOR_DECLARATION
    PipewireOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
    virtual ~PipewireOutput() noexcept;
    static core::Parameters configure();
    void on_event(uint32_t id, const char *type, const struct spa_dict *props);
    void on_event_removed(uint32_t id);
    void on_process();
private:

    virtual core::pFrame do_special_single_step(core::pRawAudioFrame frame) override;
    virtual bool set_param(const core::Parameter& param) override;

    bool init();
    void destroy();

    PipewireOutputContext pipewire_data_;
    size_t sink_;
    // format_t format_;
    // size_t samples_;
    size_t sample_size_;
    // size_t sample_rate_;
    // size_t channels_;
    bool pipewire_ready_;

    std::map<size_t, PipewireDevice> devices_;
};

} /* namespace pipewire */
} /* namespace yuri */
#endif /* PIPEWIREOUTPUT_H_ */
