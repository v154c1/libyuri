/*!
 * @file 		TwopcProtocolCoordinator.h
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		26. 3. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef TWOPCPROTOCOLCOORDINATOR_H_
#define TWOPCPROTOCOLCOORDINATOR_H_

#include "yuri/core/Module.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"
#include "TwopcProtocol.h"
#include "yuri/core/utils/StateTransitionTable.h"
#include <random>

namespace yuri {
namespace synchronization {

enum class CoordinatorState{
    initial = 0,
    collecting,
    voting
};

enum class CoordinatorEvent{
    start = 0,
    vote,
    perform,
    abort
};


class TwopcProtocolCoordinator:
        public yuri::core::IOThread,
        public StateTransitionTable<CoordinatorEvent, CoordinatorState>,
        public event::BasicEventProducer,
        public event::BasicEventConsumer
{
public:

    IOTHREAD_GENERATOR_DECLARATION
    TwopcProtocolCoordinator(log::Log &log_, core::pwThreadBase parent,const core::Parameters &parameters);
    virtual ~TwopcProtocolCoordinator() noexcept;

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

    void prepare_frame();

    void process_replies();

    void send_abort_req();

    void send_perform_req();


    void reinc();

    bool is_able_do_decisison();

    bool do_decision();

    struct Confirmation{

        Confirmation() : positive_num(0), negative_num(0), expected_num(0) {}

        void reset(){
            positive_num=0;
            negative_num=0;
        }

        uint64_t positive_num;
        uint64_t negative_num;
        uint64_t expected_num;
    };


private:

    std::mt19937 gen_;
    std::uniform_int_distribution<uint64_t> dis_;
    const uint64_t  id_;

    index_t frame_no_;
    CoordinatorEvent curr_event_;

    size_t cohorts_n_;
    int confirmation_pct_;
    bool is_strict_;
    bool variable_cohorts_;
    bool default_frame_index_;
    duration_t wait_for_replies_;
    index_t max_frame_delay;

    Confirmation confirm_;
    yuri::core::pFrame frame_;
    std::unordered_map<uint64_t, index_t> cohorts_;
};

}
}

#endif /* TWOPCPROTOCOLCOORDINATOR_H_ */
