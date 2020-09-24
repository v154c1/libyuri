//
// Created by neneko on 11/4/19.
//

#ifndef YURI2_RAWAVFILEPLAYLIST_H
#define YURI2_RAWAVFILEPLAYLIST_H

#include <yuri/event/BasicEvent.h>
#include "RawAVFile.h"

namespace yuri {
    namespace rawavfile {
        class RawAVFilePlaylist : public RawAVFile {
        public:
            IOTHREAD_GENERATOR_DECLARATION

            static core::Parameters configure();

            RawAVFilePlaylist(const log::Log &_log, core::pwThreadBase parent, const core::Parameters &parameters);

            virtual ~RawAVFilePlaylist() noexcept;

            virtual bool set_param(const core::Parameter &param) override;

            virtual bool do_process_event(const std::string &event_name, const event::pBasicEvent &event) override;

        private:
            virtual void run() override;

            virtual bool has_next_filename() override;

            virtual std::string get_next_filename() override;

            std::vector<std::string> playlist_;
            int playlist_index_;
        };

    }
}


#endif //YURI2_RAWAVFILEPLAYLIST_H
