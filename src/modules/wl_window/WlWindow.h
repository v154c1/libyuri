/*!
 * @file 		WlWindow.h
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		24.08.2023
 * @copyright	Institute of Intermedia, CTU in Prague, 2023
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef WLWINDOW_H_
#define WLWINDOW_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/gl/GL.h"

namespace yuri {
    namespace wl_window {
        class WlWindow : public core::IOThread {
        public:
            IOTHREAD_GENERATOR_DECLARATION

            static core::Parameters configure();

            WlWindow(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);

            virtual ~WlWindow() noexcept;

        private:
            //public:
            //    bool step() override;

        protected:
            void run() override;

        private:
            //    virtual core::pFrame do_simple_single_step(core::pFrame frame) override;
            virtual bool set_param(const core::Parameter &param) override;

        public:
            struct pimpl_t;

            void draw();

        private:
            gl::GL gl_;
            geometry_t geometry_;

            std::string title_;
            bool fullscreen_;

            std::unique_ptr<pimpl_t> pimpl_;
            timestamp_t counter_start_;
            size_t counter_ = 0;
            core::pFrame last_frame_;
            core::pConvert converter_;
            std::vector<format_t> supported_formats_;
        };
    } /* namespace wl_window */
} /* namespace yuri */
#endif /* WLWINDOW_H_ */
