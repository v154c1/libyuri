/*!
 * @file 		PipePolicies.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		25.11.2012
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2012 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef PIPEPOLICIES_H_
#define PIPEPOLICIES_H_
#include "yuri/core/utils/new_types.h"
#include "yuri/core/frame/Frame.h"
#include "yuri/core/parameter/Parameters.h"
#include "yuri/core/utils/assign_parameters.h"
#include <deque>
#include <cassert>
#include <random>
namespace yuri {
namespace core {

namespace pipe {

/*!
 * \brief Policy for pipes without any storage limit
 */
template<bool blocking>
class UnlimitedPolicy {
public:
	static Parameters configure() {
		Parameters p;
		p.set_description("Pipe storing an unlimited number of frames");
		return p;
	}
protected:
	UnlimitedPolicy(const Parameters&) {}
	~UnlimitedPolicy() noexcept {}
	bool impl_push_frame(const pFrame &frame)
	{
		frames_.push_back(frame);
		return true;
	}
	pFrame impl_pop_frame()
	{
		pFrame frame;
		if (frames_.empty()) return frame;
		frame = frames_.front();
		frames_.pop_front();
		return frame;
	}
	size_t impl_get_size() const {
		return frames_.size();
	}
	bool impl_is_full() const noexcept {
		return false;
	}
private:
	virtual void drop_frame(const pFrame& frame) = 0;

	std::deque<pFrame> frames_;
};

/*!
 * Policy for pipes able to hold only a single frame
 */

template<bool blocking>
class SingleFramePolicy {
public:
	static Parameters configure() {
		Parameters p;
		p.set_description(std::string("Pipe storing only single frame")+(blocking?" (blocking).":"."));
		return p;
	}
protected:
	SingleFramePolicy(const Parameters&) {}
	~SingleFramePolicy() noexcept {}
	EXPORT bool impl_push_frame(const pFrame &frame);
	pFrame impl_pop_frame()
	{
		pFrame frame = frame_;
		frame_.reset();
		return frame;
	}
	size_t impl_get_size() const {
		return frame_?1:0;
	}
	bool impl_is_full() const noexcept {
		return frame_?true:false;
	}
private:
	virtual void drop_frame(const pFrame& frame) = 0;
	pFrame frame_;
};

/*!
 * Policy for pipes able to store limited number of frames
 */
template<bool blocking>
class SizeLimitedPolicy {
public:
	static Parameters configure() {
			Parameters p;
			p.set_description(std::string("Pipe limited by total size of frames stored")+(blocking?" (blocking).":"."));
			p["size"]["Max size to store (in bytes)"]=1048576;
			return p;
		}
protected:
	SizeLimitedPolicy(const Parameters& parameters):actual_size_(0),max_size_(0),last_was_full_(false)
	{
		/*! @TODO: Process parameters */
		max_size_= parameters["size"].get<size_t>();
	}
	virtual ~SizeLimitedPolicy() noexcept {}
	void set_size_limit(yuri::size_t max_size)
	{
		max_size_ = max_size;
	}
	EXPORT bool impl_push_frame(const pFrame &frame);
	pFrame impl_pop_frame()
	{
		pFrame frame;
		if (frames_.empty()) return frame;
		frame = frames_.front();
		frames_.pop_front();
		actual_size_-= frame->get_size();
		last_was_full_ = false;
		return frame;
	}
	size_t impl_get_size() const {
		return frames_.size();
	}
	bool impl_is_full() const noexcept {
		return actual_size_ == max_size_ || last_was_full_;
	}
private:
	virtual void drop_frame(const pFrame& frame) = 0;
	std::deque<pFrame> frames_;
	yuri::size_t actual_size_;
	yuri::size_t max_size_;
	bool last_was_full_;
};

/*!
 * @brief Policy for pipes able to store only limited amount of data
 */
template<bool blocking>
class CountLimitedPolicy {
public:
	static Parameters configure() {
		Parameters p;
		p.set_description(std::string("Pipe limited by number of frames stored")+(blocking?" (blocking).":"."));
		p["count"]["Max. number of frames to store"]=10;
		return p;
	}
protected:
	CountLimitedPolicy(const Parameters& parameters):max_count_(0),
	first_index_(0),count_(0)
	{
		max_count_=parameters["count"].get<size_t>();
		if (max_count_ < 1) {
			max_count_ = 1;
		}
		frames_.resize(max_count_);
	}
	virtual ~CountLimitedPolicy() noexcept {}

	EXPORT bool impl_push_frame(const pFrame &frame);
	pFrame impl_pop_frame()
	{
		pFrame frame;
		if (count_ == 0) return frame;
		frame = std::move(frames_[first_index_]);
		// No explicit reset of frames_[first_index_] needed, it will assigned next before next use.
		++first_index_;
		--count_;
		if (first_index_ >= max_count_) {
			first_index_ = 0;
		}
		return frame;
	}
	size_t impl_get_size() const {
		return count_;
	}
	bool impl_is_full() const noexcept {
		return count_ == max_count_;
	}
private:
	virtual void drop_frame(const pFrame& frame) = 0;
	std::vector<pFrame> frames_;
	yuri::size_t max_count_;
	yuri::size_t first_index_;
	yuri::size_t count_;
};

/*!
 * Policy for pipes able to hold only a single frame.
 * This kind of pipe is intended for testing.
 * Frames are thrown away by pipe randomly.
 */

template<bool blocking>
class UnreliableSingleFramePolicy {
public:
    static Parameters configure() {
        Parameters p;
        p.set_description(std::string("Pipe storing only single frame, dropping frames with defined probability")+(blocking?" (blocking).":"."));
        p["probability"]["Probability, that the frame will be lost <0.0, 1.0>"]=0.5;
        return p;
    }

protected:
    UnreliableSingleFramePolicy(const Parameters& parameters): generator_(std::random_device()()), distribution_(0.0, 1.0) {
    	for (const auto& param: parameters) {
    		assign_parameters (param.second)
					(probability_, "probability");
    	}
    }
    ~UnreliableSingleFramePolicy() noexcept {}
    EXPORT bool impl_push_frame(const pFrame &frame);
    pFrame impl_pop_frame()
    {
        pFrame frame = frame_;
        frame_.reset();
        return frame;
    }
    size_t impl_get_size() const {
        return frame_?1:0;
    }
    bool impl_is_full() const noexcept {
        return frame_?true:false;
    }
private:
    virtual void drop_frame(const pFrame& frame) = 0;
    std::mt19937 generator_;
    std::uniform_real_distribution<float> distribution_;
    double probability_;
    pFrame frame_;
};


}

} /* namespace core */
} /* namespace yuri */
#endif /* PIPEPOLICIES_H_ */
