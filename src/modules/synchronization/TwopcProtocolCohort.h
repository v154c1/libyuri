/*!
 * @file 		TwopcProtocolCohort.h
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		26. 3. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef TWOPCPROTOCOLCOHORT_H_
#define TWOPCPROTOCOLCOHORT_H_

#include "yuri/core/Module.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"
#include "TwopcProtocol.h"
#include "yuri/core/utils/StateTransitionTable.h"
#include <random>
namespace yuri {
namespace synchronization {

enum class CohortState{
    initial = 0,
    collecting,
    voiting,
    prepared
};

enum class CohortEvent{
    prepare,
    start,
    vote_yes,
    vote_no,
    perform,
    abort
};

class TwopcProtocolCohort:
        public yuri::core::IOThread,
        public StateTransitionTable<CohortEvent, CohortState>,
        public event::BasicEventProducer,
        public event::BasicEventConsumer
{
public:

    IOTHREAD_GENERATOR_DECLARATION
    TwopcProtocolCohort(log::Log &log_, core::pwThreadBase parent,const core::Parameters &parameters);
    virtual ~TwopcProtocolCohort() noexcept;

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

protected:

    void define_transition_table() override;

    void wait_for_prepare();

    void prepare_frame();

    void send_vote();

    void wait_for_decision();

    void perform();


private:
    std::mt19937 gen_;
    std::uniform_int_distribution<uint64_t> dis_;
    const uint64_t id_;
    uint64_t id_coordinator_;

    index_t local_frame_no_;
    index_t global_frame_no_;
    CohortEvent curr_event_;
    bool default_frame_index_;
    duration_t waiting_for_frame_;


    yuri::core::pFrame frame_;
};

}
}

#endif /* TWOPCPROTOCOLCOHORT_H_ */
