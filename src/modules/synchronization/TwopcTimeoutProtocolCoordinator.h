/*!
 * @file 		TwopcTimeoutProtocolCoordinator.h
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		26. 3. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef TWOPCTIMEOUTPROTOCOLCOORDINATOR_H_
#define TWOPCTIMEOUTPROTOCOLCOORDINATOR_H_

#include "yuri/core/Module.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"
#include "TwopcProtocol.h"
#include "yuri/core/utils/StateTransitionTable.h"
#include <random>

namespace yuri {
namespace synchronization {


enum class TimeoutCoordinatorState{
    initial = 0,
    collecting,
    voiting
};

enum class TimeoutCoordinatorEvent{
    start=0,
    vote,
    perform,
    abort,
    reinc
};


class TwopcTimeoutProtocolCoordinator:
        public yuri::core::IOThread,
        public StateTransitionTable<TimeoutCoordinatorEvent,TimeoutCoordinatorState>,
        public event::BasicEventProducer,
        public event::BasicEventConsumer
{
public:

    IOTHREAD_GENERATOR_DECLARATION
    TwopcTimeoutProtocolCoordinator(log::Log &log_, core::pwThreadBase parent,const core::Parameters &parameters);
    virtual ~TwopcTimeoutProtocolCoordinator() noexcept;

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

    bool acceptable_cohorts_state();

    bool do_decision();

    void reinc();

    void prepare_frame();

    void process_replies();

    void do_abort();

    void do_perform();

    struct Confirmation{

        Confirmation() : positive_num(0), negative_num(0) {}

        void reset(){
            positive_num = 0;
            negative_num = 0;
        }

        uint64_t positive_num;
        uint64_t negative_num;
    };


private:
    std::mt19937 gen_;
    std::uniform_int_distribution<uint64_t> distrib_;
    const uint64_t id_;

    index_t frame_no_;
    double fps_;
    bool is_strict_;
    index_t frame_delay_;
    bool default_frame_index_;
    duration_t timeout_;
    TimeoutCoordinatorEvent curr_event_;
    bool initialize_;

    Timer synch_timeout_;
    Confirmation confirm_;
    std::unordered_map<uint64_t, index_t> cohorts_;
    yuri::core::pFrame frame_;
};

}
}

#endif /* TWOPCTIMEOUTPROTOCOLCOORDINATOR_H_ */
