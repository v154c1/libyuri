/*!
 * @file 		JackOutput.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		19.03.2014
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#include "JackOutput.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include <cstdlib>
#include <cmath>
#include <yuri/core/utils/assign_events.h>

namespace yuri {
    namespace jack {


        IOTHREAD_GENERATOR(JackOutput)

        core::Parameters JackOutput::configure() {
            core::Parameters p = base_type::configure();
            p.set_description("JackOutput");
            p["channels"]["Number of output channels"] = 2;
            p["allow_different_frequencies"]["Ignore sampling frequency from input frames"] = false;
            p["connect_to"]["Specify where to connect the outputs to (e.g. 'system')"] = "";
            p["buffer_size"]["Size of internal buffer"] = 1048576;
            p["client_name"]["Name of the JACK client"] = "yuri";
            p["start_server"]["Start server is it's not running."] = false;
            p["blocking"]["Blocking mode when buffer is full."] = false;
            p["auto_connect"]["Automatically connect (to specified or default client)"] = true;
            p["clamp"]["Clamp the values to <1.0,1.0> when using gains"] = true;
            p["reconnect"]["Reconnect ot jackd if disconnected. Setting to false will terminate on disconnect"] = true;
            p["allow_unconnected"]["Allow the module start without an active connection to jackd, Forces reconnect"] = false;
            return p;
        }


        namespace {
            std::map<jack_status_t, std::string> error_codes = {
                    {JackFailure,       "Overall operation failed."},
                    {JackInvalidOption, "The operation contained an invalid or unsupported option."},
                    {JackNameNotUnique, "The desired client name was not unique. With the JackUseExactName option this situation is fatal. Otherwise, the name was modified by appending a dash and a two-digit number in the range \"-01\" to \"-99\". The jack_get_client_name() function will return the exact string that was used. If the specified client_name plus these extra characters would be too long, the open fails instead."},
                    {JackServerStarted, "The JACK server was started as a result of this operation. Otherwise, it was running already. In either case the caller is now connected to jackd, so there is no race condition. When the server shuts down, the client will find out."},
                    {JackServerFailed,  "Unable to connect to the JACK server."},
                    {JackServerError,   "Communication error with the JACK server."},
                    {JackNoSuchClient,  "Requested client does not exist."},
                    {JackLoadFailure,   "Unable to load internal client"},
                    {JackInitFailure,   "Unable to initialize client"},
                    {JackShmFailure,    "Unable to access shared memory"},
                    {JackVersionError,  "Client's protocol version does not match"},
                    {JackBackendError,  "JackBackendError"},
                    {JackClientZombie,  "JackClientZombie"}
            };

            std::string get_error_string(jack_status_t err) {
                if (!err) return "No error";
                auto it = error_codes.find(err);
                if (it != error_codes.end()) return it->second;

                std::string msg;
                for (const auto &e: error_codes) {
                    if (err & e.first) msg += e.second + ", ";
                }
                if (!msg.empty()) {
                    return msg;
                }
                return "Unknown";
            }

            int process_audio_wrapper(jack_nframes_t nframes, void *arg) {
                auto j = reinterpret_cast<JackOutput *>(arg);
                return j->process_audio(nframes);
            }

            void jack_shutdown_wrapper(jack_status_t /* code */, const char *reason, void *arg) {
                auto j = reinterpret_cast<JackOutput *>(arg);
                j->notify_jack_shutdown(reason);
            }

            template<typename Target, typename Source>
            Target convert_sample(Source);


            template<>
            float convert_sample(int16_t value) {
                return static_cast<float>(value) / std::numeric_limits<int16_t>::max();
            }

            template<>
            float convert_sample(int32_t value) {
                return static_cast<float>(value) / std::numeric_limits<int32_t>::max();
            }

            template<>
            float convert_sample(uint8_t value) {
                return 2.0f * static_cast<float>(value) / std::numeric_limits<uint8_t>::max() - 1.0;
            }

            template<>
            float convert_sample(uint16_t value) {
                return 2.0f * static_cast<float>(value) / std::numeric_limits<uint16_t>::max() - 1.0;
            }

            template<>
            float convert_sample(uint32_t value) {
                return 2.0f * static_cast<float>(value) / std::numeric_limits<uint32_t>::max() - 1.0;
            }

            template<>
            float convert_sample(float value) {
                return value;
            }


            template<typename Target, class Source, typename src_fmt>
            void store_samples(std::vector<buffer_t<Target>> &buffers, const src_fmt *in, size_t nframes,
                               size_t in_channels) {
                const Source *in_frames = reinterpret_cast<const Source *>(in);
                const size_t copy_channels = std::min(in_channels, buffers.size());
                if (copy_channels < buffers.size()) {
                    for (size_t c = 0; c < (buffers.size() - copy_channels); ++c) {
                        buffers[c + copy_channels].push_silence(nframes);
                    }
                }
                for (size_t i = 0; i < nframes; ++i) {
                    for (size_t c = 0; c < copy_channels; ++c) {
                        buffers[c].push(convert_sample<Target>(*(in_frames + c)));
                    }
                    in_frames += in_channels;
                }
            }

            template<typename Target, class Source, typename src_fmt>
            void store_samples_with_gain(std::vector<buffer_t<Target>> &buffers, const std::vector<float> &gains,
                                         const src_fmt *in, size_t nframes, size_t in_channels, bool clamp_values) {
                const Source *in_frames = reinterpret_cast<const Source *>(in);
                const size_t copy_channels = std::min(in_channels, buffers.size());
                if (copy_channels < buffers.size()) {
                    for (size_t c = 0; c < (buffers.size() - copy_channels); ++c) {
                        buffers[c + copy_channels].push_silence(nframes);
                    }
                }
                if (clamp_values) {
                    for (size_t i = 0; i < nframes; ++i) {
                        for (size_t c = 0; c < copy_channels; ++c) {
                            buffers[c].push(
                                    clip_value(convert_sample<Target>(*(in_frames + c)) * gains[c], -1.0f, 1.0f));
                        }
                        in_frames += in_channels;
                    }
                } else {
                    for (size_t i = 0; i < nframes; ++i) {
                        for (size_t c = 0; c < copy_channels; ++c) {
                            buffers[c].push(convert_sample<Target>(*(in_frames + c)) * gains[c]);
                        }
                        in_frames += in_channels;
                    }
                }
            }

            inline float gain_to_db(float gain) {
                return 20.0f * std::log10(gain);
            }

            inline float db_to_gain(float db) {
                return std::pow(10.0f, db / 20.0f);
            }
        }


        JackOutput::JackOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters) :
                base_type(log_, parent, std::string("jack_output")), event::BasicEventConsumer(log), handle_(nullptr),
                client_name_("yuri_jack"), channels_(2), allow_different_frequencies_(false), buffer_size_(1048576),
                start_server_(false), blocking_(false) {
            IOTHREAD_INIT(parameters)
            if (channels_ < 1) {
                throw exception::InitializationFailed("Invalid number of channels");
            }
            if (allow_unconnected_ && !reconnect_) {
                log[log::info] << "Forcing reconnect as allow_unconnected is allowed";
                reconnect_ = true;
            }
            if (!connect_to_jackd()) {
                if (allow_unconnected_) {
                    log[log::info] << "Failed to connect to jackd, waiting for server to start";
                } else {
                    throw exception::InitializationFailed("Failed to initialize connection to jackd");
                }
            }
            using namespace core::raw_audio_format;
            set_supported_formats(
                    {unsigned_8bit, unsigned_16bit, unsigned_32bit, signed_16bit, signed_32bit, float_32bit});
        }

        JackOutput::~JackOutput() noexcept {
        }

        core::pFrame JackOutput::do_special_single_step(core::pRawAudioFrame frame) {
            jack_nframes_t sample_rate = jack_get_sample_rate(handle_.get());
            if (sample_rate != frame->get_sampling_frequency() && !allow_different_frequencies_) {
                log[log::warning] << "Frame has different sampling rate ("
                                  << frame->get_sampling_frequency() << ") than JACKd ("
                                  << sample_rate << "), ignoring";
                return {};
            }

            size_t nframes = frame->get_sample_count();
//	const int16_t * in_frames = reinterpret_cast<const int16_t*>(frame->data());

            const size_t in_channels = frame->get_channel_count();


            std::unique_lock<std::mutex> lock(data_mutex_);
            // Calling process events under the data mutex, it could mess with gains_
            process_events();
            using namespace core::raw_audio_format;
            auto empty = buffers_[0].empty();
            if (empty < nframes) {
                if (!blocking_) {
                    log[log::warning] << "Not enough space in the buffer, overwriting " << (nframes - empty)
                                      << " frames";
                } else {
                    while (still_running() && empty < nframes) {
                        if (!still_running()) {
                            request_end();
                            return {};
                        }
                        buffer_cv_.wait_for(lock, std::chrono::microseconds(get_latency()));
                        empty = buffers_[0].empty();
                    }
                }
            }
            if (gains_.empty()) {
                switch (frame->get_format()) {
                    case unsigned_8bit:
                        store_samples<jack_default_audio_sample_t, uint8_t>(buffers_, frame->data(), nframes,
                                                                            in_channels);
                        break;
                    case signed_16bit:
                        store_samples<jack_default_audio_sample_t, int16_t>(buffers_, frame->data(), nframes,
                                                                            in_channels);
                        break;
                    case unsigned_16bit:
                        store_samples<jack_default_audio_sample_t, uint16_t>(buffers_, frame->data(), nframes,
                                                                             in_channels);
                        break;
                    case signed_32bit:
                        store_samples<jack_default_audio_sample_t, int32_t>(buffers_, frame->data(), nframes,
                                                                            in_channels);
                        break;
                    case unsigned_32bit:
                        store_samples<jack_default_audio_sample_t, uint32_t>(buffers_, frame->data(), nframes,
                                                                             in_channels);
                        break;
                    case float_32bit:
                        store_samples<jack_default_audio_sample_t, float>(buffers_, frame->data(), nframes,
                                                                          in_channels);
                        break;
                    default:
                        log[log::warning] << "Unsupported frame format";
                        break;
                }
            } else {
                // When we have gains specified, we have to have them fo every channel
                if (gains_.size() < in_channels) {
                    gains_.resize(in_channels, 1.0f);
                    log[log::info] << "Resized gains to " << gains_.size() << " values";
                }
                switch (frame->get_format()) {
                    case unsigned_8bit:
                        store_samples_with_gain<jack_default_audio_sample_t, uint8_t>(buffers_, gains_, frame->data(),
                                                                                      nframes, in_channels, clamp_);
                        break;
                    case signed_16bit:
                        store_samples_with_gain<jack_default_audio_sample_t, int16_t>(buffers_, gains_, frame->data(),
                                                                                      nframes, in_channels, clamp_);
                        break;
                    case unsigned_16bit:
                        store_samples_with_gain<jack_default_audio_sample_t, uint16_t>(buffers_, gains_, frame->data(),
                                                                                       nframes, in_channels, clamp_);
                        break;
                    case signed_32bit:
                        store_samples_with_gain<jack_default_audio_sample_t, int32_t>(buffers_, gains_, frame->data(),
                                                                                      nframes, in_channels, clamp_);
                        break;
                    case unsigned_32bit:
                        store_samples_with_gain<jack_default_audio_sample_t, uint32_t>(buffers_, gains_, frame->data(),
                                                                                       nframes, in_channels, clamp_);
                        break;
                    case float_32bit:
                        store_samples_with_gain<jack_default_audio_sample_t, float>(buffers_, gains_, frame->data(),
                                                                                    nframes, in_channels, clamp_);
                        break;
                    default:
                        log[log::warning] << "Unsupported frame format";
                        break;
                }
            }
            return {};
        }

        int JackOutput::process_audio(jack_nframes_t nframes) {
            std::unique_lock<std::mutex> lock(data_mutex_);
            const size_t copy_count = std::min<size_t>(buffers_[0].size(), nframes);
            for (size_t i = 0; i < buffers_.size(); ++i) {
                if (!ports_[i]) continue;
                jack_default_audio_sample_t *data = reinterpret_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(
                        ports_[i].get(), nframes));
                buffers_[i].pop(data, copy_count);
                std::fill(data + copy_count, data + nframes, 0.0f);
                buffer_cv_.notify_all();
            }
            const auto missing = nframes - copy_count;
            if (missing > 0) {
                log[log::warning] << "Missing " << missing << " frames, filled with zeros";
            }
            return 0;
        }

        bool JackOutput::set_param(const core::Parameter &param) {
            if (assign_parameters(param)
                    (channels_, "channels")
                    (allow_different_frequencies_, "allow_different_frequencies")
                    (connect_to_, "connect_to")
                    (buffer_size_, "buffer_size")
                    (client_name_, "client_name")
                    (start_server_, "start_server")
                    (blocking_, "blocking")
                    (auto_connect_, "auto_connect")
                    (reconnect_, "reconnect")
                    (allow_unconnected_, "allow_unconnected")
                    ) {
                return true;
            } else return base_type::set_param(param);

        }

        namespace {
            template<class F>
            bool process_vector_event(const event::pBasicEvent &event, std::vector<float> &gains, F &&func) {
                if (const auto &vec_event = std::dynamic_pointer_cast<event::EventVector>(event)) {
                    if (vec_event->size() > gains.size()) {
                        gains.resize(vec_event->size(), 1.0f);
                    }
                    std::transform(vec_event->cbegin(), vec_event->cend(), gains.begin(),
                                   std::forward<F>(func));
                    return true;
                }
                return false;
            }
        }

        bool JackOutput::do_process_event(const std::string &event_name, const event::pBasicEvent &event) {
            log[log::info] << "event " << event_name;
            if (event_name == "gains") {
                // Directly specifies gains
                if (!process_vector_event(event, gains_, [](const event::pBasicEvent &event) {
                    return event::lex_cast_value<float>(event);
                })) {
                    log[log::warning] << "gains event has to receive a vector";
                    return false;
                }
            }
            if (event_name == "volume") {
                // Receives a vector of gains specified in dB.
                if (!process_vector_event(event, gains_, [](const event::pBasicEvent &event) {
                    return db_to_gain(event::lex_cast_value<float>(event));
                })) {
                    log[log::warning] << "volume event has to receive a vector";
                    return false;
                }
                log[log::info] << "Gain for channel 0: " << gains_[0] << " (" << gain_to_db(gains_[0]) << " dB)";
                return true;
            }
            return false;
        }

        bool JackOutput::connect_to_jackd() {
            jack_status_t status;
            jack_options_t options = start_server_ ? JackNullOption : JackNoStartServer;
            // Store ports and handle to local variables, move to members only on successfull connect
            decltype(handle_) handle = {jack_client_open(client_name_.c_str(), options, &status),
                                        [](jack_client_t *p) { if (p)jack_client_close(p); }};
            decltype(ports_) ports;

            if (!handle) {
                log[log::error] << "Failed to connect to JACK server: " << get_error_string(status);
                return false;
            }

            if (status & JackServerStarted) {
                log[log::info] << "Jack server was started";
            }

            if (status & JackNameNotUnique) {
                client_name_ = jack_get_client_name(handle.get());
                log[log::warning] << "Client name wasn't unique, we got new name from server instead: '" << client_name_
                                  << "'";
            }

            log[log::info] << "Connected to JACK server";

            buffers_.resize(channels_, buffer_t<jack_default_audio_sample_t>(buffer_size_));

            if (jack_set_process_callback(handle.get(), process_audio_wrapper, this) != 0) {
                log[log::error] << "Failed to set process callback!";
                return false;
            }

            jack_on_info_shutdown(handle.get(), jack_shutdown_wrapper, this);

            for (size_t i = 0; i < channels_; ++i) {
                std::string port_name = "output" + lexical_cast<std::string>(i);
                auto port = jack_port_register(handle.get(), port_name.c_str(), JACK_DEFAULT_AUDIO_TYPE,
                                               JackPortIsOutput, 0);
                if (!port) {
                    log[log::error] << "Failed to allocate output port";
                    return false;
                }
                // store raw pointer to handle to avoid possible race condition on move below.
                const auto handle_ptr = handle.get();
                ports.push_back(
                        {port, [handle_ptr](jack_port_t *p) { if (p) { jack_port_unregister(handle_ptr, p); }}});
                log[log::info] << "Opened port " << port_name;
            }

            if (jack_activate(handle.get()) != 0) {
                log[log::error] << "Failed to activate jack";
                return false;
            }
            log[log::info] << "Jack client activated";
            if (auto_connect_) {
                const char **system_ports = nullptr;
                if (connect_to_.empty()) {
                    system_ports = jack_get_ports(handle.get(), nullptr, nullptr, JackPortIsPhysical | JackPortIsInput);
                } else {
                    system_ports = jack_get_ports(handle.get(), connect_to_.c_str(), nullptr, JackPortIsInput);
                }
                if (!system_ports) {
                    log[log::warning] << "No suitable input ports found";
                } else {
                    for (size_t i = 0; i < ports.size(); ++i) {
                        if (!system_ports[i]) break;
                        if (jack_connect(handle.get(), jack_port_name(ports[i].get()), system_ports[i])) {
                            log[log::warning] << "Failed connect to output port " << i;
                        } else {
                            log[log::info] << "Connected port " << jack_port_name(ports[i].get()) << " to "
                                           << system_ports[i];
                        }
                    }
                    jack_free(system_ports);
                }
            }
            handle_ = std::move(handle);
            ports_ = std::move(ports);
            // We also have to fix the deleters here!
            for (auto &port: ports_) {
                port.get_deleter() = [=](jack_port_t *p) {
                    if (p && handle_) {
                        jack_port_unregister(handle_.get(), p);
                    }
                };
            }
            return true;
        }

        void JackOutput::notify_jack_shutdown(const char *reason) {
            log[log::info] << "Jackd shutdown (" << reason << ")";
            jackd_down_ = true;

        }

        bool JackOutput::step() {
            if (jackd_down_) {
                log[log::info] << "Releasing jackd resources";
                jackd_down_ = false;
                ports_.clear();
                handle_.reset();
            }
            if (!handle_) {
                // Disconnected from server
                if (!reconnect_) {
                    log[log::fatal] << "Not connected to jackd server and reconnect notallowed, quitting";
                    request_end(yuri::core::yuri_exit_interrupted);
                    return false;
                }
                if (connect_to_jackd()) {
                    log[log::info] << "Successfully reconnected to jackd";
                } else {
                    log[log::warning] << "Failed to reconnect to jackd";
                    return true;
                }
            }
            return base_type::step();
        }

    } /* namespace jack_output */
} /* namespace yuri */
