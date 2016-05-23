/*!
 * @file 		PipePolicies.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		25.11.2012
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2012 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "PipePolicies.h"
#include <cassert>

namespace yuri {
namespace core {
namespace pipe {

template<>
bool SingleFramePolicy<false>::impl_push_frame(const pFrame &frame)
{
	drop_frame(frame_);
	frame_ = frame;
	return true;
}
template<>
bool SingleFramePolicy<true>::impl_push_frame(const pFrame &frame)
{
	if (frame_) return false;
	frame_ = frame;
	return true;
}

template<>
bool SizeLimitedPolicy<false>::impl_push_frame(const pFrame &frame)
{
	frames_.push_back(frame);
	actual_size_+=frame->get_size();
	while (actual_size_>max_size_) {
		assert(frames_.size());
		pFrame f = frames_.front();
		frames_.pop_front();
		actual_size_-=f->get_size();
		drop_frame(f);
	}
	return true;
}
template<>
bool SizeLimitedPolicy<true>::impl_push_frame(const pFrame &frame)
{
	if ((actual_size_ + frame->get_size()) > max_size_) {
		last_was_full_ = true;
		return false;
	}
	frames_.push_back(frame);
	actual_size_+=frame->get_size();
	return true;
}

namespace {
yuri::size_t get_next_position(yuri::size_t start, yuri::size_t count, yuri::size_t max_count)
{
	return (start + count) % max_count;
}

}
template<>
bool CountLimitedPolicy<false>::impl_push_frame(const pFrame &frame)
{
	assert(count_ <= max_count_);
	if (count_ >= max_count_) {
		drop_frame(frames_[first_index_]);
		frames_[first_index_++]=frame;
		if (first_index_>=max_count_) first_index_ = 0;
	} else {
		frames_[get_next_position(first_index_, count_, max_count_)]=frame;
		++count_;
	}
	return true;
}

template<>
bool CountLimitedPolicy<true>::impl_push_frame(const pFrame &frame)
{
	assert(count_ <= max_count_);
	if (count_ >= max_count_) {
		return false;
	} else {
		frames_[get_next_position(first_index_, count_, max_count_)]=frame;
		++count_;
	}
	return true;
}

template<>
bool UnreliableSingleFramePolicy<false>::impl_push_frame(const pFrame &frame)
{
	if(distribution_(generator_) <= probability_)
		return true;
	drop_frame(frame_);
    frame_ = frame;
    return true;
}
template<>
bool UnreliableSingleFramePolicy<true>::impl_push_frame(const pFrame &frame)
{
    if (frame_) return false;
    if(distribution_(generator_) <= probability_)
    		return true;
    frame_ = frame;
    return true;
}



}
} /* namespace core */
} /* namespace yuri */
