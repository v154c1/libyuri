//
// Created by neneko on 11/4/19.
//

#include "RawAVFilePlaylist.h"
#include "yuri/core/Module.h"
#include <vector>
#include "yuri/event/EventHelpers.h"
#include "yuri/core/utils/assign_events.h"

namespace yuri {
    namespace rawavfile {

        IOTHREAD_GENERATOR(RawAVFilePlaylist)

        core::Parameters RawAVFilePlaylist::configure() {
            auto p = RawAVFile::configure();

            p["allow_empty"] = true;
            p["playlist"]["Set playlist"].set_value(std::make_shared<event::EventVector>());

            return p;
        }

        RawAVFilePlaylist::RawAVFilePlaylist(const yuri::log::Log &_log, yuri::core::pwThreadBase parent,
                                             const core::Parameters &parameters) : RawAVFile(_log, parent, parameters),
                                                                                   playlist_index_{0} {
            IOTHREAD_INIT(parameters)

        }

        RawAVFilePlaylist::~RawAVFilePlaylist() {

        }

        void RawAVFilePlaylist::run() {
            RawAVFile::run();
        }

        bool RawAVFilePlaylist::set_param(const core::Parameter &param) {
            return RawAVFile::set_param(param);
        }

        bool RawAVFilePlaylist::do_process_event(const std::string &event_name, const event::pBasicEvent &event) {
            if (event_name == "playlist") {
                if (event->get_type() != event::event_type_t::vector_event) {
                    log[log::warning] << "Playlist has to be a vector "
                                      << static_cast<int>(event->get_type());
                    return false;
                }
                auto vec = event::get_value<event::EventVector>(event);
                playlist_.clear();
                std::transform(vec.begin(), vec.end(), std::back_inserter(playlist_), [](const event::pBasicEvent &e) {
                    return event::lex_cast_value<std::string>(e);
                });

                return true;
            }
            if (event_name == "reset") {
                this->reset_indices();
                return RawAVFile::do_process_event(event_name, event);
            }
            if (assign_events(event_name, event)
                (playlist_index_, "playlist_position")) {
                this->reset_indices();
                return RawAVFile::do_process_event("reset", std::make_shared<event::EventBool>(true));
//                return true;
            }

            return RawAVFile::do_process_event(event_name, event);
        }

        bool RawAVFilePlaylist::has_next_filename() {
            log[log::debug] << "has " << playlist_.size() << " videos";
            return !playlist_.empty();
        }

        std::string RawAVFilePlaylist::get_next_filename() {
            if (playlist_index_ >= playlist_.size()) {
                playlist_index_ %= playlist_.size();
            }
            log[log::debug] << "Returning " << playlist_[playlist_index_];
            emit_event("playlist_position", playlist_index_);
            return playlist_[playlist_index_++];
        }
    }
}