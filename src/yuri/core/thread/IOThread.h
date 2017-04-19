/*!
 * @file 		IOThread.h
 * @author 		Zdenek Travnicek
 * @date 		31.5.2008
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2008 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef BASICIOTHREAD_H_
#define BASICIOTHREAD_H_

#ifdef YURI_LINUX
#include <sched.h>
#endif
#include "yuri/core/forward.h"
#include "yuri/core/utils/time_types.h"
#include "yuri/core/utils/Timer.h"
#include <vector>
#include <string>

#include "yuri/core/pipe/PipeNotification.h"
#include "yuri/core/thread/PipeConnector.h"
//#include "yuri/core/BasicIOMacros.h"
#include "yuri/core/thread/ThreadBase.h"

namespace yuri {
namespace core {

/* ****************************************************************************
 * 							Support macros
 **************************************************************************** */

#define IOTHREAD_GENERATOR_DECLARATION                                                                                                                         \
    static yuri::core::pIOThread generate(yuri::log::Log& log, yuri::core::pwThreadBase parent, const yuri::core::Parameters& parameters);
#define IOTHREAD_GENERATOR(cls)                                                                                                                                \
    yuri::core::pIOThread cls::generate(yuri::log::Log& log, yuri::core::pwThreadBase parent, const yuri::core::Parameters& parameters)                        \
    {                                                                                                                                                          \
        try {                                                                                                                                                  \
            return std::make_shared<cls>(log, parent, parameters);                                                                                             \
        } catch (std::exception & e) {                                                                                                                         \
            throw yuri::exception::InitializationFailed(std::string(#cls) + " constructor failed: " + e.what());                                               \
        }                                                                                                                                                      \
    }
#define IOTHREAD_INIT(parameters) set_params(configure().merge(parameters));

class IOThread : public ThreadBase, public PipeNotifiable {
public:
    /*!
     * Prepares default configuration of the class.
     *
     * Classes inheriting from IOThread should implement own @em configure method
     * and merge it's parameters with those from IOThread.
     *
     * @return Parameter object with supported parameters and default values.
     */
    EXPORT static Parameters configure();

    /*!
     * Constructor
     *
     * @param log_ 		yuri::log::Log object used to initialize internal logger
     * @param parent 	Parent object (can be empty)
     * @param inp		Number of input ports (can be changed later)
     * @param outp		Number of output ports (can be changed later)
     * @param id		Name of the class
     */
    EXPORT IOThread(const log::Log& log_, pwThreadBase parent, position_t inp, position_t outp, const std::string& id = "IO");

    /*!
     * Destructor
     */
    EXPORT virtual ~IOThread() noexcept;
    /* ****************************************************************************
     * 							Input/Output
     **************************************************************************** */
    /*!
     * @return 			Actual number of input ports
     */
    EXPORT position_t get_no_in_ports();

    /*!
     * @return 			Actual number of output ports
     */
    EXPORT position_t get_no_out_ports();

    /*!
     * Connects a pipe into an input port
     * @param index				Index of input pipe
     * @param pipe				The pipe to connect
     */
    EXPORT void connect_in(position_t index, pPipe pipe);
    /*!
     * Connects a pipe into an output port
     * @param index				Index of output pipe
     * @param pipe				The pipe to connect
     */
    EXPORT void connect_out(position_t index, pPipe pipe);

    /* ****************************************************************************
     * 							Parameters
     **************************************************************************** */
    /*!
     * Sets a single parameter for the class.
     * Classes inheriting from IOThread should override this method to proccess
     * own parameters, and call IOThread::set_param() for unknown parameters.
     *
     * @param parameter			Parameter to set
     * @return 	tru if parameter was processed successfully, false otherwise
     */
    EXPORT virtual bool set_param(const Parameter& parameter) override;

    /* ****************************************************************************
     * 							Protected API
     **************************************************************************** */
protected:
    /*!
     * Main loop for the thread. Classes with own loop (inputs, classes
     * processing only events) should override this method.
     */
    EXPORT virtual void run() override;

    /*!
     * Single step of the class logic. Classes using the default IOThread::run
     * method should implement own login in this method.
     *
     * Step is called when there are data available on any input and may be
     * also called spuriously.
     *
     * @return false if the class wants to end processing, true otherwise
     */
    EXPORT virtual bool step();

    /*!
     * Pushes a frame into output pipe @em index
     *
     * @param index 			Index of output pipe
     * @param frame				Frame to push
     * @return true if frame was empty, pushed successfully, or the output pipe is not connected.
     * 			If the pipe is full and not accepting frames, it returns false.
     */
    EXPORT bool push_frame(position_t index, pFrame frame);

    /*!
     * Reads frame from input pipe @em index
     *
     * @param index				Index of input pipe
     * @return A frame if there's frame available in the pipe, empty pFrame otherwise.
     */
    EXPORT pFrame pop_frame(position_t index);

    /*!
     * Changes the number of input and output pipes.
     *
     * @param inp				New number of input pipes. Set to negative number to preserve current count.
     * @param outp				New number of output pipes. Set to negative number to preserve current count.
     */
    EXPORT virtual void resize(position_t inp, position_t outp);

    /*!
     * Returns a number of input ports. This method doesn't lock @port_lock_.
     * Use this method only when working under @em port_lock_ (for example
     * when overriding @em do_connect_in), otherwise use @em get_no_in_ports.
     *
     * @return Actual number of input ports.
     */
    EXPORT position_t do_get_no_in_ports();
    /*!
     * Returns a number of output ports. This method doesn't lock @port_lock_.
     * Use this method only when working under @em port_lock_ (for example
     * when overriding @em do_connect_in), otherwise use @em get_no_out_ports.
     *
     * @return Actual number of output ports.
     */
    EXPORT position_t do_get_no_out_ports();

    /*!
     * Closes and disconnects all output pipes
     */
    EXPORT void close_pipes();

    /*!
     * Sets class latency. This setting has by default effect only for classes
     * using IOThread::run(), but child classes are encouraged to use
     * this latency settings as well.
     * The latency set by this class is mainly used for timeouts when waiting
     * for input frames. It @em step method hasn't been called for time longer
     * that this latency, it will be called even when there's no data on input.
     *
     * @param lat 				New latency
     */
    EXPORT void set_latency(duration_t lat) { latency_ = lat; }

    /*!
     * @return Current latency value.
     */
    EXPORT duration_t get_latency() { return latency_; }

    /*!
     * Checks whether there's any data available in any input pipe.
     *
     * @return true if there's data available in at least one input pipe,
     * 				false otherwise
     */
    EXPORT bool pipes_data_available();

    /*!
     * Implementation of @em connect_in method.
     * Child classes that needs to hook on pipes being connected (for example
     * for growing port count) can override this method.
     *
     *  @param position			Pipe index
     *  @param pipe				The pipe to connect
     */
    EXPORT virtual void do_connect_in(position_t position, pPipe pipe);

    /*!
     * Implementation of @em connect_out method.
     * Child classes that needs to hook on pipes being connected (for example
     * for growing port count) can override this method.
     *
     *  @param position			Pipe index
     *  @param pipe				The pipe to connect
     */
    EXPORT virtual void do_connect_out(position_t position, pPipe pipe);

private:
    position_t                 in_ports_;
    position_t                 out_ports_;
    mutex                      port_lock_;
    std::vector<PipeConnector> in_;
    std::vector<PipeConnector> out_;

    duration_t          latency_;
    std::atomic<size_t> active_pipes_;

    yuri::size_t              fps_stats_;
    std::vector<yuri::size_t> streamed_frames_;
    std::vector<yuri::size_t> frame_sizes_;
    std::vector<timestamp_t>  first_frame_;
    Timer                     pts_timer_;
    std::vector<size_t>       next_indices_;
};
}
}
#endif /*BASICIOTHREAD_H_*/
