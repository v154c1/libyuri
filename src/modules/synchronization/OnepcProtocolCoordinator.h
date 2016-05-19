/*!
 * @file 		OnepcProtocolCoordinator.h
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		4. 5. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef ONEPCPROTOCOLCOORDINATOR_H_
#define ONEPCPROTOCOLCOORDINATOR_H_

#include "yuri/core/Module.h"
#include "yuri/core/thread/IOFilter.h"
#include "yuri/event/BasicEventProducer.h"

namespace yuri {
namespace synchronization {

class OnepcProtocolCoordinator:
        public yuri::core::IOFilter,
        public event::BasicEventProducer {
public:

    IOTHREAD_GENERATOR_DECLARATION
    OnepcProtocolCoordinator(log::Log &log_, core::pwThreadBase parent,const core::Parameters &parameters);
    virtual ~OnepcProtocolCoordinator() noexcept;

    /**
     * Set up default parameters
     */
    static core::Parameters configure();

    core::pFrame do_simple_single_step(core::pFrame frame) override;

    /**
     * Set parameters from the command-line and XML file
     * @param parameter default parameters
     * @return true if the parameter is set
     */
    virtual bool set_param(const core::Parameter &parameter) override;

protected:

    event::pBasicEvent prepare_event(const uint64_t& id_sender, const index_t& data);

private:

    std::mt19937 gen_;
    std::uniform_int_distribution<uint64_t> dis_;
    const uint64_t id_;
    index_t frame_no_;
    bool use_index_frame_;
};

}
}

#endif /* ONEPCPROTOCOLCOORDINATOR_H_ */
