/*!
 * @file 		RawAudioFrame.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		21.10.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef RAWAUDIOFRAME_H_
#define RAWAUDIOFRAME_H_

#include "AudioFrame.h"
#include "yuri/core/utils/uvector.h"

namespace yuri {
namespace core {

class RawAudioFrame;
typedef std::shared_ptr<RawAudioFrame> pRawAudioFrame;

class RawAudioFrame: public AudioFrame
{
public:
	EXPORT RawAudioFrame(format_t format, size_t channel_count, size_t sampling_frequency);
	EXPORT ~RawAudioFrame() noexcept;


	/*!
	 * Creates an audio frame with uninitilized data
	 * @param format
	 * @param channel_count
	 * @param sampling_frequency
	 * @param sample_count
	 * @return
	 */
	EXPORT static pRawAudioFrame create_empty(format_t format, size_t channel_count, size_t sampling_frequency, size_t sample_count);

	/*!
	 * Creates an audio frame filled with data copied from a buffer
	 * @param format
	 * @param channel_count
	 * @param sampling_frequency
	 * @param data
	 * @param size
	 * @return
	 */
	EXPORT static pRawAudioFrame create_empty(format_t format, size_t channel_count, size_t sampling_frequency, const uint8_t* data, size_t size);

	/*!
	 * Creates an audio frame filled with data moved from from an uvector
	 * @param format
	 * @param channel_count
	 * @param sampling_frequency
	 * @param data
	 * @return
	 */
	EXPORT static pRawAudioFrame create_empty(format_t format, size_t channel_count, size_t sampling_frequency, uvector<uint8_t>&& data);


	/*!
	 * Creates an audio frame filled with data copied from a buffer
	 * @param format
	 * @param channel_count
	 * @param sampling_frequency
	 * @param data
	 * @param size
	 * @return
	 */
	template<class T>
	static  pRawAudioFrame create_empty(format_t format, size_t channel_count, size_t sampling_frequency, const T* data, size_t size)
	{
		return create_empty(format, channel_count, sampling_frequency, reinterpret_cast<const uint8_t*>(data), size * sizeof(T));
	}

	/*!
	 * Creates an audio frame filled that with data from a buffer with a specified deleter
	 * @param format
	 * @param channel_count
	 * @param sampling_frequency
	 * @param data
	 * @param size
	 * @param deleter
	 * @return
	 */
	template<class Deleter>
	static  pRawAudioFrame create_empty(format_t format, size_t channel_count, size_t sampling_frequency, uint8_t* data, size_t size, Deleter deleter)
	{
		return create_empty(format, channel_count, sampling_frequency, uvector<uint8_t>{data, size, deleter});
	}

	template<class T>
	static  pRawAudioFrame create_empty(format_t format, size_t channel_count, size_t sampling_frequency, const std::vector<T>& data)
	{
		return create_empty(format, channel_count, sampling_frequency, reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T));
	}

	template<class T, size_t N>
	static  pRawAudioFrame create_empty(format_t format, size_t channel_count, size_t sampling_frequency, const std::array<T, N>& data)
	{
		return create_empty(format, channel_count, sampling_frequency, reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T));
	}

	template<class T>
	static  pRawAudioFrame create_empty(format_t format, size_t channel_count, size_t sampling_frequency, const uvector<T>& data)
	{
		return create_empty(format, channel_count, sampling_frequency, reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T));
	}


	uint8_t *				data() { return data_.data(); }
	const uint8_t *			data() const { return data_.data(); }

	void					resize(size_t size) { data_.resize(size); }
	size_t					size() const { return data_.size(); }

	void					set_data(uvector<uint8_t>&&data);

	size_t					get_sample_count() const { return data_.size() * 8 / sample_size; };
	size_t					get_sample_size() const { return sample_size; }
private:
	/*!
	 * Implementation of copy, should be implemented in node classes only.
	 * @return Copy of current frame
	 */
	EXPORT virtual pFrame	do_get_copy() const;
	/*!
	 * Implementation od get_size() method
	 * @return Size of current frame
	 */
	EXPORT virtual size_t	do_get_size() const noexcept;

	uvector<uint8_t>		data_;
	size_t					sample_size;
};

}
}


#endif /* RAWAUDIOFRAME_H_ */
