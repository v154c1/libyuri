/*!
 * @file 		LogProxy.h
 * @author 		Zdenek Travnicek
 * @date 		11.2.1013
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef LOGPROXY_H_
#define LOGPROXY_H_
#include "yuri/core/utils/new_types.h"
#include "LogUtils.h"
#include <ostream>
#include <sstream>
#include <optional>

namespace yuri {
namespace log {

template<
        class CharT,
        class Traits = std::char_traits<CharT>
>
struct generic_out_stream {
    typedef std::basic_string<CharT, Traits> string_t;
    typedef std::basic_ostream<CharT, Traits> stream_t;
    typedef std::basic_stringstream<CharT, Traits> sstream_t;
    typedef CharT char_t;

    virtual ~generic_out_stream() = default;

    virtual void put_message(const LogMessageInfo& info, const sstream_t& sstream) = 0;
};
/*!
 * @brief 		Wrapper struct for std::basic_ostream providing locking
 */
template<
    class CharT,
    class Traits = std::char_traits<CharT>
>
struct guarded_stream: public generic_out_stream<CharT, Traits> {
	using stream_t = typename generic_out_stream<CharT, Traits>::stream_t;
    using sstream_t = typename generic_out_stream<CharT, Traits>::sstream_t;

    void put_message(const LogMessageInfo& info, const sstream_t& sstream) override{
        yuri::lock_t _(mutex_);

            print_level(str_, info.level, info.output_flags);
            if (info.output_flags & show_thread_id) {
                str_ << info.uid << ": ";
            }
            str_ << info.time << info.logger_name;
            str_ << sstream.rdbuf();
            str_ << str_.widen('\n');
    }

	guarded_stream(stream_t& str):str_(str) {}
	~guarded_stream() noexcept = default;
private:
	stream_t& str_;
	yuri::mutex mutex_;
};

template<
        class CharT,
        class Traits = std::char_traits<CharT>
>
struct LogProxyData {
    using gstream_t = generic_out_stream<CharT, Traits>;
    using sstream_t = typename generic_out_stream<CharT, Traits>::sstream_t;

    LogMessageInfo info;
    gstream_t& out;
    sstream_t buffer;
};

/**
 * @brief Proxy for output stream
 *
 * LogProxy wraps an output stream and ensures that lines are written correctly
 * even when logging concurrently from several threads
 */
template<
    class CharT,
    class Traits = std::char_traits<CharT>
>
class LogProxy {
private:

public:
    typedef std::basic_ostream<CharT, Traits>& (*iomanip_t)(std::basic_ostream<CharT, Traits>&);
	using gstream_t = typename LogProxyData<CharT, Traits>::gstream_t;

	/*!
	 * @param	str_	Reference to a @em guarded_stream to write the messages
	 * @param	dummy	All input is discarded, when dummy is set to true
	 */
	LogProxy(gstream_t& out, LogMessageInfo info):data_{{std::move(info),out,{}}} {

    }

    LogProxy() = default;

	LogProxy(const LogProxy&) = delete;
	/*!
	* @brief			Move constructor. Invalides the original LogProxy object
	*/

	LogProxy(LogProxy&& other) noexcept = delete;//noexcept
//		:stream_(std::move(other.stream_)), info_(std::move(other.info_)) {
////		if (info_) {
////            // TODO: Can't we just swap bufers?
////			buffer_ << other.buffer_.str();
////		}
//	}
#ifndef REF_QUALIFIED_MF_UNSUPPORTED
	/*!
	 * @brief			Provides ostream like operator << for inserting messages
	 * @tparam	T		Type of message to insert
	 * @param	val_	Message to write
	 */
	template<typename T>
	LogProxy& operator<<(const T& val_) &
	{
		if (data_) {
            (data_->buffer) << val_;
		}
		return *this;
	}

	/*!
	 * @brief			Provides ostream like operator << for inserting messages
	 * @tparam	T		Type of message to insert
	 * @param	val_	Message to write
	 */
	template<typename T>
	LogProxy&& operator<<(const T& val_) &&
	{
        if (data_) {
            (data_->buffer) << val_;
        }
		return std::move(*this);
	}
#else
	/*!
	 * @brief			Provides ostream like operator << for inserting messages
	 * @tparam	T		Type of message to insert
	 * @param	val_	Message to write
	 */
	template<typename T>
	LogProxy& operator<<(const T& val_)
	{
		if (!dummy_) {
			buffer_ << val_;
		}
		return *this;
	}
#endif
	/*!
	 * @brief 			Overloaded operator<< for manipulators
	 *
	 * We are storing internally to std::stringstream, which won't accept std::endl,
	 * so this method simply replaces std::endl with newlines.
	 * @param	manip	Manipulator for the stream
	 */
	LogProxy& operator<<(iomanip_t manip)
	{
        if (data_) {
            auto& d = *data_;
            // We can't call endl on std::stringstream, so let's filter it out
			if (manip==static_cast<iomanip_t>(std::endl)) {
                d.buffer << d.buffer.widen('\n');
            } else {
                d.buffer << manip;
            }
		}
		return *this;
	}

	~LogProxy() noexcept {
		if (data_) {
            auto& d = *data_;
            d.out.put_message(d.info, d.buffer);
		}
	}


    private:
    std::optional<LogProxyData<CharT, Traits>> data_;
};

}
}


#endif /* LOGPROXY_H_ */
