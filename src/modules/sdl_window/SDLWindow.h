/*!
 * @file 		SDLWindow.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		24.09.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SDLWINDOW_H_
#define SDLWINDOW_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"
#ifdef YURI_WIN
#include <Windows.h>
#endif
#include "SDL.h"
#ifdef  YURI_SDL_OPENGL
#include "yuri/gl/GL.h"
#endif

namespace yuri {
namespace sdl_window {

class SDLWindow: public core::SpecializedIOFilter<core::RawVideoFrame>,
public event::BasicEventConsumer, public event::BasicEventProducer
{
	using base_type = core::SpecializedIOFilter<core::RawVideoFrame>;
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	SDLWindow(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~SDLWindow() noexcept;
private:
	
	virtual void run() override;
	virtual bool step() override;
	virtual core::pFrame do_special_single_step(core::pRawVideoFrame frame) override;
	virtual bool set_param(const core::Parameter& param) override;
	void process_sdl_events();
	void sdl_resize(resolution_t);
	bool prepare_rgb_overlay(const core::pRawVideoFrame& frame);
	bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;
	resolution_t	resolution_;
	resolution_t    last_frame_res_;
	bool			fullscreen_;
	bool			default_keys_;
	bool			use_gl_;
	SDL_Surface*	surface_;
	std::unique_ptr<SDL_Overlay, std::function<void(SDL_Overlay*)>>	overlay_;
	std::unique_ptr<SDL_Surface, std::function<void(SDL_Surface*)>>	rgb_surface_;
	int				sdl_bpp_;
	std::string		title_;
	bool			decorations_;
	bool			show_cursor_;
	bool			keep_aspect_gl_;
	coordinates_t	position_;
	std::string		display_;
	std::string		display_str_;
	timestamp_t		last_click_;
#ifdef  YURI_SDL_OPENGL
	gl::GL				gl_;
	std::string			transform_shader_;
	std::string			color_map_shader_;
	bool				flip_x_;
	bool				flip_y_;
	bool				read_back_;
	int					shader_version_;
#endif
};

} /* namespace sdl_window */
} /* namespace yuri */
#endif /* SDLWINDOW_H_ */
