/*!
 * @file 		JackOutput.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		19.03.2014
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#ifndef JACKOUTPUT_H_
#define JACKOUTPUT_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include <jack/jack.h>

namespace yuri {
namespace jack {

template<typename T>
struct buffer_t {
	using data_type = T;
	std::vector<data_type> data;
	size_t start;
	size_t end;
	buffer_t(size_t size):data(size,0),start(0),end(0) {}
	~buffer_t() noexcept {}
	size_t size() const noexcept {
		if (end < start) {
			return end + data.size() - start;
		}
		return end - start;
	}
	size_t empty() const noexcept {
		return data.size() - size();
	}
	inline void push(data_type value) {
		if (end==data.size()) {
			data[0]=value;
			end=1;
//			if (start==0) start = 1;
		} else {
			data[end++]=value;
			if (end == start) ++start;
			if (start == data.size()) start = 0;
		}
	}
	inline void push_silence(size_t count) {
		for (size_t i = 0;i<count;++i) push(0.0);
	}
	inline void pop(data_type* dest, size_t count) {
		if (end == start) return;
		if (end < start) {
			size_t copy_count = std::min(data.size() - start, count);
			std::copy(&data[start], &data[start+copy_count], dest);
			start += copy_count;
			if (start >= data.size()) start = 0;
			if (count == copy_count) return;
			return pop(dest+copy_count, count - copy_count);
		}
		size_t copy_count = std::min(end - start, count);
		std::copy(&data[start], &data[start+copy_count], dest);
		start += copy_count;
	}
};

class JackOutput: public core::SpecializedIOFilter<core::RawAudioFrame>
{
	using base_type = core::SpecializedIOFilter<core::RawAudioFrame>;
	using handle_t = std::unique_ptr<jack_client_t, std::function<void(jack_client_t*)>>;
	using port_t = std::unique_ptr<jack_port_t, std::function<void(jack_port_t*)>>;
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	JackOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~JackOutput() noexcept;
	int process_audio(jack_nframes_t nframes);
private:
	
	virtual core::pFrame do_special_single_step(core::pRawAudioFrame frame) override;
	virtual bool set_param(const core::Parameter& param) override;

	handle_t handle_;
	std::vector<port_t> ports_;
	std::string client_name_;
	std::string port_name_;
	std::string connect_to_;
	size_t channels_;
	bool allow_different_frequencies_;
	size_t buffer_size_;
	std::vector<buffer_t<jack_default_audio_sample_t>> buffers_;
	std::mutex	data_mutex_;
	bool start_server_;
	bool blocking_;

	std::condition_variable buffer_cv_;

};

} /* namespace jack_output */
} /* namespace yuri */
#endif /* JACKOUTPUT_H_ */
