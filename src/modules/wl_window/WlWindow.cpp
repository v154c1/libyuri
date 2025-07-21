/*!
 * @file 		WlWindow.cpp
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		24.08.2023
 * @copyright	Institute of Intermedia, CTU in Prague, 2023
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "WlWindow.h"
#include "yuri/core/Module.h"
#include "yuri/core/utils/make_unique.h"

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <cstring>
#include "xdg-shell-client-protocol.h"
#include "yuri/core/thread/Convert.h"

namespace yuri {
    namespace wl_window {
        IOTHREAD_GENERATOR(WlWindow)

        MODULE_REGISTRATION_BEGIN("wl_window")
            REGISTER_IOTHREAD("wl_window", WlWindow)
        MODULE_REGISTRATION_END()

        core::Parameters WlWindow::configure() {
            core::Parameters p = core::IOThread::configure();
            p.set_description("WlWindow");
            p["geometry"]["Geometry (overrides resolution and position)"] = "1920x1080+0+0";
            p["title"]["WIndow title"] = "[Yuri] WL Window";
            p["fullscreen"]["Start fullscreen"] = false;
            return p;
        }


        struct WlWindow::pimpl_t {
            pimpl_t(log::Log &log, WlWindow &win, geometry_t geometry) : log(log), win(win), geometry(geometry) {
            }

            void init();

            void resize(resolution_t res);

            void stop();

            log::Log &log;
            WlWindow &win;
            geometry_t geometry;
            wl_display *display = nullptr;
            struct wl_compositor *compositor = nullptr;
            //            struct wl_shm *shm = nullptr;
            //            struct wl_shm_pool *pool = nullptr;
            //            struct wl_output *output = nullptr;
            wl_region *region = nullptr;

            struct xdg_wm_base *xdg_wm_base = nullptr;
            /* Objects */
            struct wl_surface *surface = nullptr;
            struct xdg_surface *xdg_surface;
            struct xdg_toplevel *xdg_toplevel;


            EGLDisplay egl_display;
            EGLConfig egl_config;
            wl_egl_window *egl_window = nullptr;
            EGLSurface egl_surface;
            EGLContext egl_context;

            bool resized_ = true;

            void create_window();

            void init_egl();

            void draw();

            void swap_buffers();

            void set_title(const std::string &name);

            void set_fullscreen(bool state);
        };

        namespace {
            void
            xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
                xdg_wm_base_pong(xdg_wm_base, serial);
                auto &state = *reinterpret_cast<WlWindow::pimpl_t *>(data);
                state.log[log::debug] << "Ping";
            }

            const struct xdg_wm_base_listener xdg_wm_base_listener = {
                .ping = xdg_wm_base_ping,
            };

            void registry_handle_global(void *data, struct wl_registry *registry,
                                        uint32_t name, const char *interface, uint32_t version) {
                auto &state = *reinterpret_cast<WlWindow::pimpl_t *>(data);
                state.log[log::debug] << "interface: '" << interface << "', version: " << version << ", name: " << name;

                if (strcmp(interface, wl_compositor_interface.name) == 0) {
                    state.compositor = reinterpret_cast<wl_compositor *>(wl_registry_bind(
                        registry, name, &wl_compositor_interface, 4));
                    state.log[log::debug] << "Bound compositor";
                } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
                    state.xdg_wm_base = reinterpret_cast<xdg_wm_base *>(wl_registry_bind(
                        registry, name, &xdg_wm_base_interface, 1));
                    printf("Bound xdg shell\n");
                    xdg_wm_base_add_listener(state.xdg_wm_base,
                                             &xdg_wm_base_listener, data);
                }
            }

            void
            registry_handle_global_remove(void */*data*/, struct wl_registry */*registry*/,
                                          uint32_t /*name*/) {
                // This space deliberately left blank
            }

            const struct wl_registry_listener
            registry_listener = {
                .global = registry_handle_global,
                .global_remove = registry_handle_global_remove,
            };

            void
            xdg_surface_configure(void *data,
                                  struct xdg_surface *xdg_surface, uint32_t serial) {
                auto &state = *reinterpret_cast<WlWindow::pimpl_t *>(data);
                xdg_surface_ack_configure(xdg_surface, serial);
                state.log[log::debug] << "XDG surface configure";
            }

            const struct xdg_surface_listener xdg_surface_listener = {
                .configure = xdg_surface_configure,
            };

            void xdg_toplevel_handle_configure(void *data,
                                               struct xdg_toplevel */*xdg_toplevel*/, int32_t w, int32_t h,
                                               struct wl_array */*states*/) {
                printf("Top level configure, w: %d, h: %d\n", w, h);
                // no window geometry event, ignore
                if (w == 0 && h == 0) {
                    return;
                }
                auto &state = *reinterpret_cast<WlWindow::pimpl_t *>(data);

                //        // window resized
                const auto new_res = resolution_t{static_cast<dimension_t>(w), static_cast<dimension_t>(h)};
                if (state.geometry.get_resolution() != new_res) {
                    state.resize(new_res);
                }
            }

            static void xdg_toplevel_handle_close(void *data,
                                                  struct xdg_toplevel */*xdg_toplevel*/) {
                // window closed, be sure that this event gets processed

                auto &state = *reinterpret_cast<WlWindow::pimpl_t *>(data);
                state.log[log::info] << "Window closed!";
                state.stop();
            }

            struct xdg_toplevel_listener xdg_toplevel_listener = {
                .configure = xdg_toplevel_handle_configure,
                .close = xdg_toplevel_handle_close,
                .configure_bounds = nullptr,
                .wm_capabilities = nullptr,

            };
        }

        void WlWindow::pimpl_t::init() {
            display = wl_display_connect(nullptr);
            if (!display) {
                throw exception::InitializationFailed("Failed to connect to Wayland display");
            }
            log[log::info] << "Connection established!";


            struct wl_registry *registry = wl_display_get_registry(display);
            wl_registry_add_listener(registry, &registry_listener, this);
            wl_display_roundtrip(display);

            if (!compositor || !xdg_wm_base) {
                throw exception::InitializationFailed("Failed to bind compositor or xdg_vm_base");
            }
            surface = wl_compositor_create_surface(compositor);

            xdg_surface = xdg_wm_base_get_xdg_surface(
                xdg_wm_base, surface);
            xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, this);
            xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
            xdg_toplevel_set_title(xdg_toplevel, "WlWindow");
            xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, this);
            //    xdg_toplevel_set_fullscreen
            wl_surface_commit(surface);

            wl_display_roundtrip(display);

            init_egl();
            create_window();
        }

        void WlWindow::pimpl_t::resize(yuri::resolution_t res) {
            log[log::info] << "Requested resize to " << res;
            wl_egl_window_resize(egl_window, res.width, res.height, 0, 0);
            wl_surface_commit(surface);
            geometry.width = res.width;
            geometry.height = res.height;
            glViewport(0, 0, geometry.width, geometry.height);
            resized_ = true;
        }

        void WlWindow::pimpl_t::create_window() {
            region = wl_compositor_create_region(compositor);
            wl_region_add(region, 0, 0, geometry.width, geometry.height);
            wl_surface_set_opaque_region(surface, region);

            egl_window = wl_egl_window_create(surface, geometry.width, geometry.height);
            if (!egl_window) {
                throw exception::InitializationFailed("Failed to create EGL window");
            }
            log[log::info] << "Created EGL window";

            egl_surface = eglCreateWindowSurface(egl_display, egl_config, egl_window, nullptr);
            if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
                log[log::info] << "Made context current";
            } else {
                throw exception::InitializationFailed("Failed to made context current");
            }

            auto shader_ver = glGetString(GL_SHADING_LANGUAGE_VERSION);
            log[log::info] << "Supported shader version: " << shader_ver;

            //            draw();
            //            swap_buffers();
        }

        void WlWindow::pimpl_t::init_egl() {
            EGLint major;
            EGLint minor;


            EGLint config_attribs[] = {
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                //                    EGL_ALPHA_SIZE, 8,
                //                    EGL_DEPTH_SIZE, 24,
                //                    EGL_BUFFER_SIZE, 32,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_NONE
            };
            static const EGLint context_ettirbs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                //                    EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
                EGL_NONE
            };

            if ((egl_display = eglGetDisplay(display)) == EGL_NO_DISPLAY) {
                throw exception::InitializationFailed("Can't create EGL display");
            }
            log[log::info] << "Created EGL display";

            if (!eglInitialize(egl_display, &major, &minor)) {
                throw exception::InitializationFailed("Failed to initialize EGL display");
            }

            log[log::info] << "EGL: major: " << major << ", minor " << minor;

            int count = 0;
            eglGetConfigs(egl_display, nullptr, 0, &count);
            log[log::info] << "EGL has " << count << " configs";

            std::vector<EGLConfig> configs(count, nullptr);
            int found = 0;
            eglChooseConfig(egl_display, config_attribs, configs.data(), count, &found);
            for (int i = 0; i < found; ++i) {
                int size = 0;
                eglGetConfigAttrib(display, configs[i], EGL_BUFFER_SIZE, &size);
                log[log::debug] << "Config " << i << ", Buffer size: " << size;
            }

            egl_config = configs[0];
            egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_ettirbs);
        }

        void WlWindow::pimpl_t::draw() {
            win.draw();
            swap_buffers();
        }

        void WlWindow::pimpl_t::swap_buffers() {
            wl_surface_damage(surface, 0, 0, 0xFFFF, 0xFFFF);
            eglSwapBuffers(egl_display, egl_surface);
        }

        void WlWindow::pimpl_t::stop() {
            win.request_end();
        }

        void WlWindow::pimpl_t::set_title(const std::string &name) {
            xdg_toplevel_set_title(xdg_toplevel, name.c_str());
        }

        void WlWindow::pimpl_t::set_fullscreen(bool state) {
            if (state) {
                xdg_toplevel_set_fullscreen(xdg_toplevel, nullptr);
            } else {
                xdg_toplevel_unset_fullscreen(xdg_toplevel);
            }
        }


        WlWindow::WlWindow(const log::Log &log_, core::pwThreadBase parent,
                           const core::Parameters &parameters) : core::IOThread(log_, parent, 1, 0,
                                                                     std::string("wl_window")), gl_(log) {
            IOTHREAD_INIT(parameters)
            gl_.log.set_flags(log.get_flags());
            gl_.shader_version_ = 300;
            gl_.use_core = true;
            gl_.shader_suffix_ = " es";
            gl_.use_lq = true;
            pimpl_ = yuri::make_unique<pimpl_t>(log, *this, geometry_);

            supported_formats_ = gl_.get_supported_formats();

            set_latency(1_ms);
        }

        WlWindow::~WlWindow() noexcept = default;

        //
        //        core::pFrame WlWindow::do_simple_single_step(core::pFrame frame) {
        //
        //            draw();
        //            pimpl_->swap_buffers();
        //        }

        bool WlWindow::set_param(const core::Parameter &param) {
            if (assign_parameters(param)
                (geometry_, "geometry")
                (title_, "title")
                (fullscreen_, "fullscreen")
            ) {
                return true;
            }
            return core::IOThread::set_param(param);
        }

        void WlWindow::draw() {
            gl_.clear();
            if (last_frame_) {
                //                log[log::info] << "Frame: " << last_frame_->get_index();
                gl_.generate_texture(0, last_frame_, false, false);
                gl_.draw_texture(0);
                //                gl_.finish_frame();
            } {
                timestamp_t now{};
                auto delta = now - counter_start_;
                ++counter_;
                if (delta > 5_s) {
                    auto fps = 1000.0 * 1_s * counter_ / delta / 1000.0;
                    log[log::info] << "FPS: " << fps;
                    counter_ = 0;
                    counter_start_ = now;
                }
            }

            //            glClearColor(0.2, 0.3, 0.4, 1.0);
            //            glClear(GL_COLOR_BUFFER_BIT);
        }

        //
        //        bool WlWindow::step() {
        //
        //            wl_display_dispatch_pending(pimpl_->display);
        ////            draw();
        ////            pimpl_->swap_buffers();
        //            return MultiIOFilter::step();
        //        }

        void WlWindow::run() {
            pimpl_->init();
            pimpl_->set_title(title_);
            pimpl_->set_fullscreen(fullscreen_);
            converter_.reset(new core::Convert(log, get_this_ptr(), core::Convert::configure()));
            add_child(converter_);
            counter_ = 0;
            counter_start_ = timestamp_t{};
            while (still_running()) {
                wait_for(get_latency());
                auto frame = pop_frame(0);
                bool redraw = pimpl_->resized_ || frame;
                if (frame) {
                    last_frame_ = converter_->convert_to_cheapest(std::move(frame), supported_formats_);
                }
                if (redraw) {
                    draw();
                    pimpl_->swap_buffers();
                    pimpl_->resized_ = false;
                }


                wl_display_dispatch_pending(pimpl_->display);
            }
            //            IOThread::run();
        }
    } /* namespace wl_window */
} /* namespace yuri */
