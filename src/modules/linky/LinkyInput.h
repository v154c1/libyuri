/*!
 * @file 		LinkyInput.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		26.09.2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2016
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef LINKYINPUT_H_
#define LINKYINPUT_H_

#include "yuri/core/thread/IOThread.h"

namespace yuri {
namespace linky {

class LinkyInput : public core::IOThread {
public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    LinkyInput(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~LinkyInput() noexcept;

private:
    virtual void run();
    virtual bool set_param(const core::Parameter& param) override;

    std::string  api_path_;
    std::string  key_;
    resolution_t resolution_;
    bool         use_jpeg_;
};

} /* namespace linky_input */
} /* namespace yuri */
#endif /* LINKYINPUT_H_ */
