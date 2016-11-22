/*!
 * @file 		LinkyOutput.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		26.09.2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2016
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef LINKYOUTPUT_H_
#define LINKYOUTPUT_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/event/BasicEventConsumer.h"
namespace yuri {
namespace linky {

class LinkyOutput : public core::SpecializedIOFilter<core::RawVideoFrame>, public event::BasicEventConsumer {
    using base_type = core::SpecializedIOFilter<core::RawVideoFrame>;

public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    LinkyOutput(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~LinkyOutput() noexcept;

private:
    virtual core::pFrame do_special_single_step(core::pRawVideoFrame frame) override;
    virtual bool set_param(const core::Parameter& param) override;
    virtual bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;

    std::string  api_path_;
    std::string  key_;
    resolution_t resolution_;
    bool         use_rgbw_;
    uint8_t      w_value_;
    bool         alpha_as_white_;
    bool         sample_;
    float        sample_border_;
};

} /* namespace linky_output */
} /* namespace yuri */
#endif /* LINKYOUTPUT_H_ */
