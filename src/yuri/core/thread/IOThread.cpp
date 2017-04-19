/*!
 * @file 		IOThread.cpp
 * @author 		Zdenek Travnicek
 * @date 		31.5.2008
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2008 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "IOThread.h"
#include "yuri/exception/NotImplemented.h"
#include "yuri/core/frame/Frame.h"
#include "yuri/core/pipe/Pipe.h"
#include "yuri/core/utils/assign_parameters.h"
#include <algorithm>
#include <stdexcept>
#include <numeric>
#include "yuri/core/utils/trace_method.h"

namespace yuri {
namespace core {

Parameters IOThread::configure()
{
    auto p                                                                        = ThreadBase::configure();
    p["fps_stats"]["Print out_ current FPS every n frames. Set to 0 to disable."] = 0;
    return p;
}

IOThread::IOThread(const log::Log& log_, pwThreadBase parent, position_t inp, position_t outp, const std::string& id)
    : ThreadBase(log_, parent, id), in_ports_(inp), out_ports_(outp), latency_(200_ms), active_pipes_(0), fps_stats_(0)

{
    TRACE_METHOD
    log.set_label(get_node_name());
    resize(inp, outp);
}

IOThread::~IOThread() noexcept
{
    TRACE_METHOD
    close_pipes();
}

void IOThread::run()
{
    TRACE_METHOD
    try {
        while (still_running()) {
            if (!active_pipes_ /*&& in_ports_ */) {
                ThreadBase::sleep(latency_);
            }
            if (in_ports_ && !pipes_data_available()) {
                wait_for(latency_);
            }
            //			log[log::verbose_debug] << "Stepping";
            if (!step())
                break;
        }
    } catch (std::runtime_error& e) {
        log[log::debug] << "Thread failed: " << e.what();
    }

    close_pipes();
}

// Dummy IOThread::step(), so inherited classes don't have to override it if not needed.
bool IOThread::step()
{
    throw std::runtime_error("This method should be never called!");
}
position_t IOThread::get_no_in_ports()
{
    lock_t _(port_lock_);
    return do_get_no_in_ports();
}
position_t IOThread::do_get_no_in_ports()
{
    return in_ports_;
}
position_t IOThread::get_no_out_ports()
{
    lock_t _(port_lock_);
    return do_get_no_out_ports();
}
position_t IOThread::do_get_no_out_ports()
{
    return out_ports_;
}

void IOThread::connect_in(position_t index, pPipe pipe)
{
    TRACE_METHOD
    lock_t _(port_lock_);
    do_connect_in(index, pipe);
}
void IOThread::do_connect_in(position_t index, pPipe pipe)
{
    TRACE_METHOD
    if (index < 0 || index >= do_get_no_in_ports())
        throw std::out_of_range("Input pipe out of Range");
    if (in_[index]) {
        log[log::debug] << "Disconnecting already connected pipe from in port " << index;
    }
    auto notify_ptr = std::dynamic_pointer_cast<PipeNotifiable>(get_this_ptr());
    in_[index]      = PipeConnector(pipe, notify_ptr, {});
    active_pipes_   = std::accumulate(in_.begin(), in_.end(), size_t{}, [](const size_t& ap, const PipeConnector& p) { return ap + (p ? 1 : 0); });
}

void IOThread::connect_out(position_t index, pPipe pipe)
{
    TRACE_METHOD
    do_connect_out(index, pipe);
}

void IOThread::do_connect_out(position_t index, pPipe pipe)
{
    TRACE_METHOD
    if (index < 0 || index >= do_get_no_out_ports())
        throw std::out_of_range("Output pipe out of Range");
    if (out_[index])
        log[log::debug] << "Disconnecting already connected pipe from out port " << index;
    auto notify_ptr = std::dynamic_pointer_cast<PipeNotifiable>(get_this_ptr());
    // Output pipe should send source notifications!
    out_[index] = PipeConnector(pipe, {}, notify_ptr);
}
bool IOThread::push_frame(position_t index, pFrame frame)
{
    TRACE_METHOD
    if (!frame)
        return true;
    const auto cur_idx = frame->get_index();
    if (static_cast<position_t>(next_indices_.size()) <= index) {
        next_indices_.resize(index + 1);
    }
    if (cur_idx == 0) {
        const auto idx = next_indices_[index]++;
        frame->set_index(idx);
    } else {
        next_indices_[index] = cur_idx + 1;
    }
    if (index >= 0 && index < get_no_out_ports() && out_[index]) {
        if (fps_stats_) {
            frame_sizes_[index] += frame->get_size();
        }
        while (!out_[index]->push_frame(std::move(frame))) {
            wait_for(latency_);
            if (!still_running())
                return false;
        }
        if (fps_stats_ && ++streamed_frames_[index] >= fps_stats_) {
            const size_t      frames = streamed_frames_[index];
            const timestamp_t start  = first_frame_[index];
            const timestamp_t now;
            const duration_t  dur   = now - start;
            const auto        brate = static_cast<double>(frame_sizes_[index]) * 1.0e3 / dur.value;
            log[log::info] << "Output " << index << " streamed " << frames << " in " << dur << ", that's " << (frames * 1e6 / dur.value) << " fps, bitrate "
                           << std::setprecision(3) << brate << " kB/s";
            first_frame_[index]     = now;
            streamed_frames_[index] = 0;
            frame_sizes_[index]     = 0;
        }
        return true;
    }
    return false;
}

pFrame IOThread::pop_frame(position_t index)
{
    TRACE_METHOD
    if (index >= 0 && index < get_no_in_ports() && in_[index])
        return in_[index]->pop_frame();
    return pFrame();
}

void IOThread::resize(position_t inp, position_t outp)
{
    TRACE_METHOD
    log[log::debug] << "Resizing to " << inp << " input ports and " << outp << " output ports.";
    if (inp >= 0)
        in_ports_ = inp;
    if (outp >= 0)
        out_ports_ = outp;

    in_.resize(in_ports_);
    out_.resize(out_ports_);
    streamed_frames_.resize(out_ports_, 0);
    first_frame_.resize(out_ports_);
    next_indices_.resize(out_ports_, 0);
    frame_sizes_.resize(out_ports_, 0);
}

void IOThread::close_pipes()
{
    TRACE_METHOD
    log[log::debug] << "Closing pipes!";
    for (auto& pipe : out_) {
        if (pipe)
            pipe->close_pipe();
        pipe.reset();
    }
}

bool IOThread::pipes_data_available()
{
    TRACE_METHOD
    for (auto& pipe : in_) {
        if (!pipe)
            continue;
        if (!pipe->is_empty())
            return true;
        if (pipe->is_finished()) {
            pipe.reset();
            active_pipes_--;
        }
    }
    return false;
}

bool IOThread::set_param(const Parameter& parameter)
{
    if (assign_parameters(parameter) //
        (fps_stats_, "fps_stats")    //
        )
        return true;
    return ThreadBase::set_param(parameter);
}
}
}

// End of File
