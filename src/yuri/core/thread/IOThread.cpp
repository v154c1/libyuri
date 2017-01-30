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

namespace yuri
{
namespace core
{

Parameters IOThread::configure()
{
	auto p = ThreadBase::configure();
	p["fps_stats"]["Print out_ current FPS every n frames. Set to 0 to disable."]=0;
	return p;
}


IOThread::IOThread(const log::Log &log_,pwThreadBase parent, position_t inp, position_t outp, const std::string& id):
	ThreadBase(log_, parent, id), in_ports_(inp), out_ports_(outp),latency_(200_ms),
	active_pipes_(0),
	fps_stats_(0)

{
	TRACE_METHOD
	//params.merge(*configure());
	log.set_label(get_node_name());
	resize(inp,outp);
}

IOThread::~IOThread() noexcept
{
	TRACE_METHOD
	close_pipes();
}

void IOThread::run()
{
	TRACE_METHOD
//	IO_THREAD_PRE_RUN
	try {
		while (still_running()) {
			if (!active_pipes_ /*&& in_ports_ */) {
				ThreadBase::sleep(latency_);
			}
			if (in_ports_ && !pipes_data_available()) {
				wait_for(latency_);
			}
			log[log::verbose_debug] << "Stepping";
			if (!step()) break;
		}
	}
	catch (std::runtime_error& e) {
		log[log::debug] << "Thread failed: " << e.what();
	}

	close_pipes();
//	IO_THREAD_POST_RUN
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
	if (index < 0 || index >= do_get_no_in_ports()) throw std::out_of_range("Input pipe out of Range");
	if (in_[index]) {
		log[log::debug] << "Disconnecting already connected pipe from in port " << index;
	}
	auto notify_ptr = std::dynamic_pointer_cast<PipeNotifiable>(get_this_ptr());
	in_[index]=PipeConnector(pipe,notify_ptr, {});
	active_pipes_ = std::accumulate(in_.begin(), in_.end(), size_t{}, [](const size_t& ap, const PipeConnector&p) {return ap + (p ? 1 : 0); });
}

void IOThread::connect_out(position_t index, pPipe pipe)
{
	TRACE_METHOD
	do_connect_out(index, pipe);
}

void IOThread::do_connect_out(position_t index, pPipe pipe)
{
	TRACE_METHOD
	if (index < 0 || index >= do_get_no_out_ports()) throw std::out_of_range("Output pipe out of Range");
	if (out_[index]) log[log::debug] << "Disconnecting already connected pipe from out port " << index;
	auto notify_ptr = std::dynamic_pointer_cast<PipeNotifiable>(get_this_ptr());
	// Output pipe should send source notifications!
	out_[index]=PipeConnector(pipe,{}, notify_ptr);
}
bool IOThread::push_frame(position_t index, pFrame frame)
{
	TRACE_METHOD
	if (!frame) return true;
	const auto cur_idx = frame->get_index();
	if (static_cast<position_t>(next_indices_.size()) <= index) {
		next_indices_.resize(index+1);
	}
	if (cur_idx == 0) {
		const auto idx = next_indices_[index]++;
		frame->set_index(idx);
	} else {
		next_indices_[index] = cur_idx+1;
	}
	if (index >= 0 && index < get_no_out_ports() && out_[index]) {
		if (fps_stats_) {
			frame_sizes_[index]+=frame->get_size();
		}
		while (!out_[index]->push_frame(std::move(frame))) {
			wait_for(latency_);
			if (!still_running()) return false;
		}
		if (fps_stats_ && ++streamed_frames_[index]>=fps_stats_) {
			const size_t frames = streamed_frames_[index];
			const timestamp_t start = first_frame_[index];
			const timestamp_t now;
			const duration_t dur = now-start;
			const auto brate = static_cast<double>(frame_sizes_[index])*1.0e3/dur.value;
			log[log::info] << "Streamed " << frames << " in " << dur << ", that's " << (frames*1e6/dur.value) << " fps, bitrate " << std::setprecision(3) << brate << " kB/s";
			first_frame_[index] = now;
			streamed_frames_[index] = 0;
			frame_sizes_[index] = 0;
		}
		return true;
	}
	return false;
}

pFrame	IOThread::pop_frame(position_t index)
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
	if (inp >= 0) in_ports_	= inp;
	if (outp>= 0) out_ports_= outp;
//	PipeConnector pc(dynamic_pointer_cast<PipeNotifiable>(get_this_ptr()));
	in_.resize(in_ports_);
	out_.resize(out_ports_);
	streamed_frames_.resize(out_ports_,0);
	first_frame_.resize(out_ports_);
	next_indices_.resize(out_ports_, 0);
	frame_sizes_.resize(out_ports_, 0);
}

void IOThread::close_pipes()
{
	TRACE_METHOD
	log[log::debug] << "Closing pipes!";
	for (auto& pipe: out_) {
		if (pipe) pipe->close_pipe();
		pipe.reset();
	}
}

bool IOThread::pipes_data_available()
{
	TRACE_METHOD
	for (auto& pipe: in_) {
		if (!pipe) continue;
		if (!pipe->is_empty()) return true;
		if (pipe->is_finished()) {
			pipe.reset();
			active_pipes_--;
		}
	}
	return false;
}

/*pBasicFrame IOThread::timestamp_frame(pBasicFrame frame)
{
#ifndef YURI_USE_CXX11
	if (pts_base==boost::posix_time::not_a_date_time) pts_base=boost::posix_time::microsec_clock::local_time();
	boost::posix_time::time_duration pts = boost::posix_time::microsec_clock::local_time() - pts_base;
	frame->set_time(pts.total_microseconds(),frame->get_dts(),frame->get_duration());
#else
	if (pts_base.time_since_epoch() == time_duration::zero()) {
		pts_base = std::chrono::steady_clock::now();
	}
	time_duration pts = std::chrono::steady_clock::now() - pts_base;
	frame->set_time(std::chrono::duration_cast<std::chrono::microseconds>(pts).count(),frame->get_dts(),frame->get_duration());
#endif
	return frame;
}
// TODO Stub, not implemented!!!!!!
//pBasicFrame IOThread::get_frame_as(yuri::sint_t index, yuri::format_t format)
//{
//	pBasicFrame frame;
//	if (index>=in_ports_ || !in_[index] || in_[index]->is_empty()) return frame;
//	if (format == YURI_FMT_NONE) return in_[index]->pop_frame();
//
//	return pBasicFrame();
//}
 * */

//void IOThread::set_affinity(yuri::ssize_t affinity)
//{
//	cpu_affinity = affinity;
//}
 /*
bool IOThread::set_params(Parameters &parameters)
{
#ifndef YURI_USE_CXX11
	std::pair<std::string,shared_ptr<Parameter> > par;
	BOOST_FOREACH(par,(parameters.params)) {
#else
	for(auto& par: parameters.params) {
#endif
		if (par.second && !set_param(*(par.second))) return false;
	}
	return true;
}*/
/* Sets one parameter.
 *
 * @param:  parameter - parameter to set
 * @return false on error (like value can't be set) and true otherwise (even for unknown parameter)
 *
 */
bool IOThread::set_param(const Parameter &parameter)
{
	if (assign_parameters(parameter)
			(fps_stats_, "fps_stats"))
		return true;
	return ThreadBase::set_param(parameter);
}

/*
pBasicFrame IOThread::allocate_empty_frame(yuri::format_t format, yuri::size_t width, yuri::size_t height, bool large)
{
	pBasicFrame pic;
	FormatInfo_t fmt = BasicPipe::get_format_info(format);
	assert(fmt->planes && !fmt->compressed); // Just to make sure the format is valid
	pic.reset(new BasicFrame(fmt->planes));
	yuri::size_t bpplane = fmt->bpp;
	for (yuri::size_t i = 0; i < fmt->planes; ++i) {
		if (fmt->planes>1) bpplane = fmt->component_depths[i];
		yuri::size_t planesize = width * height * bpplane / fmt->plane_x_subs[i] / fmt->plane_y_subs[i] / 8;
		if (!large) {
			pic->get_plane(i).resize(planesize);
		} else {
			FixedMemoryAllocator::memory_block_t block = FixedMemoryAllocator::get_block(planesize);
			assert(block.first);
			//std::copy(mem,mem+size,&block.first[0]);
			pic->get_plane(i).set(block.first,planesize,block.second);
		}
		pic->get_plane(i).resize(planesize);
	}
	pic->set_parameters(format,width,height);
	return pic;
}
pBasicFrame IOThread::allocate_empty_frame(size_t size, bool large)
{
	pBasicFrame frame = yuri::make_shared<BasicFrame>(1);
	if (!large) {
		frame->get_plane(0).resize(size);
	} else {
		FixedMemoryAllocator::memory_block_t block = FixedMemoryAllocator::get_block(size);
		assert(block.first);
		frame->get_plane(0).set(block.first,size,block.second);
	}
	return frame;
}

bool IOThread::connect_threads(shared_ptr<BasicIOThread> src, yuri::sint_t s_idx, shared_ptr<BasicIOThread> target, yuri::sint_t t_idx, Log &log,std::string name, shared_ptr<Parameters> params)
{
	shared_ptr<Parameters> p = BasicPipe::configure();
	if (params) p->merge(*params);
	shared_ptr<BasicPipe> pipe = BasicPipe::generator(log,name,*p);
	target->connect_in(s_idx,pipe);
	src->connect_out(t_idx,pipe);
	return true;
}

*/


}
}

// End of File
