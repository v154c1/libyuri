/*!
 * @file 		OnepcProtocolCohort.h
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		4. 5. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */


#ifndef ONEPCPROTOCOLCOHORT_H_
#define ONEPCPROTOCOLCOHORT_H_

#include "yuri/core/Module.h"
#include "yuri/core/forward.h"
#include "yuri/event/BasicEventConsumer.h"
#include <unordered_map>

namespace yuri {
namespace synchronization {

enum class CentralTendencyType{
    impr_average,
    mode,
    none
};

class OnepcProtocolCohort:
        public yuri::core::IOThread,
        public event::BasicEventConsumer {
public:

    IOTHREAD_GENERATOR_DECLARATION
    OnepcProtocolCohort(log::Log &log_, core::pwThreadBase parent,const core::Parameters &parameters);
    virtual ~OnepcProtocolCohort() noexcept;

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

    double const& get_fps() const noexcept { return fps_; }

    bool set_fps(const core::pFrame frame);

    std::unordered_map<int64_t, int64_t> const& get_delays() const noexcept {
        return delays_;
    }

    void set_delays(std::unordered_map<int64_t, int64_t> delays) noexcept {
        delays_ = delays;
    }


    /**
     * Get current delay according to a specified central tendency type.
     * @brief get_delay
     * @return current delay
     */
    virtual int get_delay();


    /**
     * the most frequent value in the data set
     * @brief mode_eql
     * @return
     */
    int calculate_mode_of_sample();

    /**
     * the sum of all measurements divided by the number of observations in the data set
     * @brief calculate_impr_average_of_sample
     * @return
     */
     int calculate_impr_average_of_sample();

private:

    bool changed_;
    index_t global_frame_no_;
    index_t local_frame_no_;
    uint64_t id_coordinator_;
    double frame_delay_;
    CentralTendencyType tendency_;
    double fps_;
    bool use_index_frame_;
    bool allow_restart_;
    std::unordered_map<int64_t, int64_t> delays_;
};

}
}

#endif /* ONEPCPROTOCOLCOHORT_H_ */
