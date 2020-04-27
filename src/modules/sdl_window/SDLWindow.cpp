/*!
 * @file 		SDLWindow.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		24.09.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "SDLWindow.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/version.h"
#include "yuri/core/utils/Timer.h"
#include <unordered_map>
#include "yuri/core/utils/assign_events.h"
#ifdef YURI_LINUX
#include "SDL_syswm.h"

#endif

namespace yuri {
namespace sdl_window {

namespace {
const std::unordered_map<format_t, Uint32> yuri_to_sdl_yuv =
	{{core::raw_format::yuyv422, SDL_YUY2_OVERLAY},
	 {core::raw_format::yvyu422, SDL_YVYU_OVERLAY},
	 {core::raw_format::uyvy422, SDL_UYVY_OVERLAY}};
// TODO: Support for planar SDL_YV12_OVERLAY and SDL_IYUV_OVERLAY


Uint32 map_yuv_yuri_to_sdl(format_t fmt) {
	auto it = yuri_to_sdl_yuv.find(fmt);
	if (it == yuri_to_sdl_yuv.end()) return 0;
	return it->second;
}

//! Supported formats for HW overlay
const std::vector<format_t> supported_formats = {
	core::raw_format::yuyv422,
	core::raw_format::yvyu422,
	core::raw_format::uyvy422,
	core::raw_format::rgb24,
	core::raw_format::bgr24,
	core::raw_format::rgb15,
	core::raw_format::bgr15,
	core::raw_format::rgb16,
	core::raw_format::bgr16,
	core::raw_format::argb32,
	core::raw_format::rgba32,
	core::raw_format::abgr32,
	core::raw_format::bgra32
};
#include "sdl_keys.impl"

std::string get_keyname(SDLKey key) {
	auto it = sdl_keys.find(key);
	if (it == sdl_keys.end()) return {};
	return it->second;
}

}


IOTHREAD_GENERATOR(SDLWindow)

MODULE_REGISTRATION_BEGIN("sdl_window")
		REGISTER_IOTHREAD("sdl_window",SDLWindow)
MODULE_REGISTRATION_END()

core::Parameters SDLWindow::configure()
{
	core::Parameters p = base_type::configure();
	p.set_description("SDLWindow");
	p["resolution"]["Resolution of output window"]=resolution_t{800,600};
	p["fullscreen"]["Start in fullscreen"]=false;
	p["opengl"]["Use OpenGL for rendering"]=false;
	p["default_keys"]["Enable default key events. This includes ESC for quit and f for fullscreen toggle."]=true;
	p["window_title"]["Window title"]=std::string();
	p["decorations"]["Window decorations"]=true;
	p["show_cursor"]["Enable or disable cursor in the window"]=true;
	p["position"]["Window position"]=coordinates_t{-1,-1};
	p["display"]["Display for the window. Warning: It may affect other threads behavior as well."]="";
#ifdef YURI_SDL_OPENGL
	p["transform_shader"]["Shader to use for texture transformations"]=std::string();
	p["color_shader"]["Shader to use for color mapping"]=std::string();
	p["flip_x"]["Flip around vertical axis"]=false;
	p["flip_y"]["Flip around horizontal axis"]=false;
	p["read_back"]["Read drawn picture back and output it"]=false;
	p["shader_version"]["Version of GLSL. Keep on default version unless you need higher."]=120;
#endif
	return p;
}


SDLWindow::SDLWindow(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::SpecializedIOFilter<core::RawVideoFrame>(log_,parent,std::string("sdl_window")),
BasicEventConsumer(log),BasicEventProducer(log),
resolution_({800,600}),fullscreen_(false),default_keys_(true),use_gl_(false),
overlay_{nullptr,[](SDL_Overlay*o){if(o) SDL_FreeYUVOverlay(o);}},
rgb_surface_{nullptr,[](SDL_Surface*s){ if(s) SDL_FreeSurface(s);}},
sdl_bpp_(32),title_(std::string("Yuri2 (")+yuri_version+")"),decorations_(true),
show_cursor_{true},position_(coordinates_t{-1, -1})
#ifdef YURI_SDL_OPENGL
,gl_(log),flip_x_(false),flip_y_(false),read_back_(false),shader_version_(120)
#endif
{
	IOTHREAD_INIT(parameters)
	set_latency(1_ms);
	if (!display_.empty()) {
		display_str_ = "DISPLAY="+display_;
		::putenv(const_cast<char*>(display_str_.c_str()));
	}
	//if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
	//	throw exception::InitializationFailed("Failed to initialize SDL");
	//}
#ifdef  YURI_SDL_OPENGL
	if (!use_gl_) {
		set_supported_formats(supported_formats);
	} else {
		set_supported_formats(gl_.get_supported_formats());
		gl_.transform_shader = transform_shader_;
		gl_.color_map_shader = color_map_shader_;
		gl_.shader_version_ = shader_version_;
		if (read_back_) {
			resize(1,1);
		}
		log[log::info] << "Set ts to:\n"<<transform_shader_;
	}
#else
	if (use_gl_) {
		log[log::warning] << "Enabled OpenGL, but OpenGL support is not enabled. Disabling";
		use_gl_ = false;
	}
	set_supported_formats(supported_formats);
#endif
}

SDLWindow::~SDLWindow() noexcept
{
	SDL_Quit();
}

void SDLWindow::run()
{
	print_id();
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
		throw exception::InitializationFailed("Failed to initialize SDL ("+std::string(SDL_GetError())+")");
	}
	if (!(surface_ = SDL_SetVideoMode(static_cast<int>(resolution_.width), static_cast<int>(resolution_.height), sdl_bpp_,
			  SDL_HWSURFACE |  SDL_DOUBLEBUF | SDL_RESIZABLE |
			  (decorations_?0:SDL_NOFRAME) |
			  (fullscreen_?SDL_FULLSCREEN:0) |
			  (use_gl_?SDL_OPENGL:0)))) {
		throw exception::InitializationFailed("Failed to set video mode");
	}
#if defined(YURI_LINUX) && defined(SDL_VIDEO_DRIVER_X11)
	if (position_.x != -1 && position_.y != -1) {
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (SDL_GetWMInfo(&info) > 0 && info.subsystem == SDL_SYSWM_X11) {
		info.info.x11.lock_func();
		XMoveWindow(info.info.x11.display, info.info.x11.wmwindow, position_.x, position_.y);
		XMapRaised(info.info.x11.display, info.info.x11.wmwindow);
		info.info.x11.unlock_func();
		}
	}
#endif
	SDL_WM_SetCaption(title_.c_str(), "yuri2");
	if (!show_cursor_) SDL_ShowCursor(SDL_DISABLE);
#ifdef YURI_SDL_OPENGL
	if (use_gl_) {
		gl_.enable_smoothing();
		gl_.setup_ortho();
	}
#endif
	IOThread::run();
	overlay_.reset();
	rgb_surface_.reset();
}

bool SDLWindow::step()
{
	process_sdl_events();
	return base_type::step();
}

core::pFrame SDLWindow::do_special_single_step(core::pRawVideoFrame frame)
{
	BasicEventConsumer::process_events();
	Timer timer;
	core::pFrame out_frame;
	const resolution_t res = frame->get_resolution();
	const dimension_t src_linesize  = PLANE_DATA(frame,0).get_line_size();
	auto it = PLANE_DATA(frame,0).begin();

	format_t format = frame->get_format();
	Uint32 sdl_fmt = map_yuv_yuri_to_sdl(format);
#ifdef  YURI_SDL_OPENGL
	if (use_gl_) {
		glDrawBuffer(GL_BACK_LEFT);
		gl_.clear();
		gl_.generate_texture(0, frame, flip_x_, flip_y_);
		gl_.draw_texture(0);
		gl_.finish_frame();
		if (read_back_) {
			out_frame = gl_.read_window(resolution_.get_geometry());
		}
		SDL_GL_SwapBuffers();
	} else
#endif
		if (sdl_fmt) {
		if (!overlay_ ||
				overlay_->w != static_cast<int>(res.width) ||
				overlay_->h != static_cast<int>(res.height) ||
				overlay_->format != sdl_fmt) {
			overlay_.reset(SDL_CreateYUVOverlay(static_cast<int>(res.width), static_cast<int>(res.height), sdl_fmt, surface_));
		}
		if (!overlay_) {
			log[log::error] << "Failed to allocate overlay";
			return {};
		}
		const dimension_t target_linesize  = static_cast<dimension_t>(overlay_->pitches[0]);
		const dimension_t copy_linesize = std::min(src_linesize, target_linesize);
//		SDL_LockYUVOverlay(overlay_.get());
		for (dimension_t line = 0; line < res.height; ++line) {
			std::copy(it, it + copy_linesize, overlay_->pixels[0] + target_linesize * line);
			it += src_linesize;
		}
//		SDL_UnlockYUVOverlay(overlay_.get());
		SDL_Rect rec={0,0,static_cast<Uint16>(resolution_.width), static_cast<Uint16>(resolution_.height)};
		SDL_DisplayYUVOverlay(overlay_.get(), &rec);
	} else if (prepare_rgb_overlay(frame)) {
		const dimension_t target_linesize  = static_cast<dimension_t>(rgb_surface_->pitch);
		const dimension_t copy_linesize = std::min(src_linesize, target_linesize);
		const dimension_t copy_lines = std::min(res.height, resolution_.height);
		for (dimension_t line = 0; line < copy_lines; ++line) {
			std::copy(it, it + copy_linesize, reinterpret_cast<uint8_t*>(rgb_surface_->pixels) + target_linesize * line);
			it += src_linesize;
		}
		SDL_Rect dest_rec={0,0,static_cast<Uint16>(resolution_.width), static_cast<Uint16>(resolution_.height)};
		SDL_Rect src_rec={0,0,static_cast<Uint16>(res.width), static_cast<Uint16>(res.height)};
//		SDL_SoftStretch(rgb_surface_.get(), &src_rec, rgb_surface2_.get(), &dest_rec);
//		SDL_BlitSurface(rgb_surface2_.get(), &dest_rec, surface_, &dest_rec);
//		SDL_SoftStretch(rgb_surface2_.get(), &dest_rec, surface_, &dest_rec);
//		SDL_SoftStretch(rgb_surface_.get(), &src_rec, surface_, &dest_rec);

		SDL_BlitSurface(rgb_surface_.get(), &src_rec, surface_, &dest_rec);
		SDL_Flip(surface_);
	} else {

		const auto& fi = core::raw_format::get_format_info(format);
		log[log::warning] << "Unsupported format '" << fi.name << "'";
	}
	log[log::debug] << "Processing took " << timer.get_duration();
	return out_frame;
}
bool SDLWindow::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			(resolution_, "resolution")
			(fullscreen_, "fullscreen")
			(default_keys_, "default_keys")
			(use_gl_, "opengl")
			(decorations_, "decorations")
			(show_cursor_, "show_cursor")
			(position_, "position")
			(display_, "display")
#if YURI_SDL_OPENGL
			(transform_shader_, "transform_shader")
			(color_map_shader_, "color_shader")
			(flip_x_, "flip_x")
			(flip_y_, "flip_y")
			(read_back_, "read_back")
			(shader_version_, "shader_version")
#endif
		) return true;

	if (param.get_name() == "window_title") {
		std::string new_title = param.get<std::string>();
		if (!new_title.empty()) title_=std::move(new_title);
	} else return base_type::set_param(param);
	return true;
}
void SDLWindow::process_sdl_events()
{
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: request_end(core::yuri_exit_interrupted);
				break;
			case SDL_VIDEORESIZE:
				sdl_resize({static_cast<dimension_t>(event.resize.w), static_cast<dimension_t>(event.resize.h)});
				break;
			case SDL_KEYDOWN:
				if (default_keys_) {
					if (event.key.keysym.sym == SDLK_ESCAPE) request_end(core::yuri_exit_interrupted);
					if (event.key.keysym.sym == SDLK_f) { fullscreen_=!fullscreen_; sdl_resize(resolution_);}
				}
				{
					const auto key = get_keyname(event.key.keysym.sym);
					emit_event("key", key);
					emit_event(key);
				}
				break;
			case SDL_MOUSEMOTION:{
				// there should be a way to disable this...
				std::vector<event::pBasicEvent> motion{
						std::make_shared<event::EventInt>(event.motion.x, 0, resolution_.width),
						std::make_shared<event::EventInt>(event.motion.y, 0, resolution_.height)};

				emit_event("mouse", std::make_shared<event::EventVector>(motion));
				emit_event("mouse_x", event.motion.x, 0, resolution_.width);
				emit_event("mouse_y", event.motion.y, 0, resolution_.height);
				}break;
			case SDL_MOUSEBUTTONDOWN:{
				emit_event("button",event.button.button);
				emit_event("button"+std::to_string(event.button.button));
				switch (event.button.button) {
					case SDL_BUTTON_LEFT: {emit_event("mouse_left");
						auto t = timestamp_t{};
						if ((t - last_click_) < 250_ms) {
							emit_event("mouse_double_click");
						}
						last_click_ = t;
						}break;
					case SDL_BUTTON_RIGHT: emit_event("mouse_right");
						break;
					case SDL_BUTTON_MIDDLE: emit_event("mouse_middle");
						break;
					case SDL_BUTTON_WHEELDOWN: emit_event("wheel_down");
						break;
					case SDL_BUTTON_WHEELUP: emit_event("wheel_up");
						break;
				}
				}break;

			default:break;
		}
	}
}

void SDLWindow::sdl_resize(resolution_t new_res)
{
	resolution_ = new_res;
	Uint32 flags = (surface_->flags & ~SDL_FULLSCREEN) | (fullscreen_?SDL_FULLSCREEN:0);
	if (surface_) surface_ = SDL_SetVideoMode(static_cast<int>(resolution_.width), static_cast<int>(resolution_.height), sdl_bpp_, flags);
	if (!use_gl_) {
		overlay_.reset();
	} else {
#ifdef YURI_SDL_OPENGL
		glViewport( 0, 0, new_res.width, new_res.height );
		gl_.setup_ortho();
#endif
	}
}

bool SDLWindow::prepare_rgb_overlay(const core::pRawVideoFrame& frame)
{
	std::tuple<Uint32, Uint32,Uint32, Uint32> masks;
	const resolution_t res = frame->get_resolution();
	format_t format = frame->get_format();
	switch (format) {
		case core::raw_format::rgb24: masks = std::make_tuple(0x0000FF,0x00FF00,0xFF0000,0);
				break;
		case core::raw_format::bgr24: masks = std::make_tuple(0xFF0000,0x00FF00,0x0000FF,0);
				break;
		case core::raw_format::rgb15: masks = std::make_tuple(0xF800,0x07C0,0x001E,0);
				break;
		case core::raw_format::bgr15: masks = std::make_tuple(0x3E00,0x07C0,0x00F8,0);
				break;
		case core::raw_format::rgb16: masks = std::make_tuple(0xF800,0x07E0,0x001F,0);
				break;
		case core::raw_format::bgr16: masks = std::make_tuple(0x1F00,0x07E0,0x00F8,0);
				break;
		case core::raw_format::argb32: masks = std::make_tuple(0x0000FF00,0x00FF0000,0xFF000000,0x00000000);
				break;
		case core::raw_format::rgba32: masks = std::make_tuple(0x000000FF,0x0000FF00,0x00FF0000,0x00000000);
				break;
		case core::raw_format::abgr32: masks = std::make_tuple(0xFF000000,0x00FF0000,0x0000FF00,0x00000000);
				break;
		case core::raw_format::bgra32: masks = std::make_tuple(0x00FF0000,0x0000FF00,0x000000FF,0x00000000);
				break;
		default: return false;
	}
	const auto& fi = core::raw_format::get_format_info(format);
	int bpp = fi.planes[0].bit_depth.first/fi.planes[0].bit_depth.second;
	if (!rgb_surface_ ||
			static_cast<dimension_t>(rgb_surface_->w) != res.width ||
			static_cast<dimension_t>(rgb_surface_->h) != res.height ||
			rgb_surface_->format->BitsPerPixel != bpp) {
		log[log::info] << "(Re)creating RGB surface with " << bpp << " bpp.";
		rgb_surface_.reset(SDL_CreateRGBSurface(SDL_SWSURFACE, static_cast<int>(res.width), static_cast<int>(res.height), static_cast<int>(bpp),
				std::get<0>(masks), std::get<1>(masks), std::get<2>(masks), std::get<3>(masks)));
//		rgb_surface2_.reset(SDL_CreateRGBSurface(SDL_SWSURFACE, resolution_.width, resolution_.height, bpp,
//				std::get<0>(masks), std::get<1>(masks), std::get<2>(masks), std::get<3>(masks)),
//				[](SDL_Surface*s){SDL_FreeSurface(s);});
	}
	if (bpp != sdl_bpp_) {
		sdl_bpp_ = static_cast<int>(bpp);
		sdl_resize(resolution_);
	}
	return true;
}

bool SDLWindow:: do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
#ifdef YURI_SDL_OPENGL
	if (assign_events(event_name, event)
			.vector_values("corner0", gl_.corners[0], gl_.corners[1])
			.vector_values("corner1", gl_.corners[2], gl_.corners[3])
			.vector_values("corner2", gl_.corners[4], gl_.corners[5])
			.vector_values("corner3", gl_.corners[6], gl_.corners[7])
			(gl_.corners[0], "x0")
			(gl_.corners[1], "y0")
			(gl_.corners[2], "x1")
			(gl_.corners[3], "y1")
			(gl_.corners[4], "x2")
			(gl_.corners[5], "y2")
			(gl_.corners[6], "x3")
			(gl_.corners[7], "y3"))

		return true;
#endif
	return false;
}
} /* namespace sdl_window */
} /* namespace yuri */

