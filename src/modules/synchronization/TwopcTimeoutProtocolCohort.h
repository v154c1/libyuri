/*!
 * @file 		TwopcTimeoutProtocolCohort.h
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		26. 3. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef TWOPCTIMEOUTPROTOCOLCOHORT_H_
#define TWOPCTIMEOUTPROTOCOLCOHORT_H_

#include "yuri/core/Module.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"
#include "TwopcProtocol.h"
#include "yuri/core/utils/StateTransitionTable.h"
#include <random>

namespace yuri {
namespace synchronization {


enum class TimeoutCohortState{
    initial=0,
    collecting,
    voiting,
    prepared
};

enum class TimeoutCohortEvent{
    start = 0,
    prepare,
    vote_yes,
    vote_no,
    perform,
    abort,
    timeout
};

class TwopcTimeoutProtocolCohort:
        public yuri::core::IOThread,
        public StateTransitionTable<TimeoutCohortEvent, TimeoutCohortState>,
        public event::BasicEventProducer,
        public event::BasicEventConsumer
{
public:

    IOTHREAD_GENERATOR_DECLARATION
    TwopcTimeoutProtocolCohort(log::Log &log_, core::pwThreadBase parent,const core::Parameters &parameters);
    virtual ~TwopcTimeoutProtocolCohort() noexcept;

    /**
     * Set up default parameters
     */
    static core::Parameters configure();

    /**
     * Set parameters from the command-line and XML file
     * @param parameter default parameters
     * @return true if the parameter is set
     */
    virtual bool set_param(const core::Parameter &parameter) override;


    virtual bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;

    virtual  void run() override;

    void define_transition_table() override;

    void prepare_frame();

    void wait_for_decision();


    void wait_for_prepare();

    void perform();

private:
    std::mt19937 gen_;
    std::uniform_int_distribution<uint64_t> distrib_;
    const uint64_t id_;
    //Keeps id of coordinator.
    // 0 means that coordinator hasn't been assigned
    uint64_t id_coordinator_;

    index_t global_frame_no_;
    index_t local_frame_no_;
    double fps_;
    TimeoutCohortEvent curr_event_;
    bool initialize_;
    bool default_frame_index_;
    duration_t timeout_;

    Timer timer_;
    yuri::core::pFrame frame_;
};

}
}

#endif /* TWOPCTIMEOUTPROTOCOLCOHORT_H_ */
