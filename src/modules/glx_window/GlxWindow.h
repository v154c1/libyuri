/*!
 * @file 		GlxWindow.h
 * @author 		<Your name>
 * @date 		25.01.2015
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#ifndef GLXWINDOW_H_
#define GLXWINDOW_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"
#include "yuri/gl/GL.h"
#include "GL/glx.h"
#include <X11/Xatom.h>
namespace yuri {
namespace glx_window {

enum class stereo_mode_t {
	none,
	quadbuffer,
	anaglyph,
	side_by_side,
	top_bottom,
};

class GlxWindow: public core::IOThread, public event::BasicEventConsumer,
public event::BasicEventProducer
{
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	GlxWindow(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~GlxWindow() noexcept;
private:

	virtual void run() override;
	virtual bool set_param(const core::Parameter& param) override;
	virtual bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;
	bool create_window();
	bool create_glx_context();
	bool show_window(bool show = true);
	bool show_cursor(bool show = false);
	void move_window(coordinates_t coord);
	void resize_window(resolution_t res);
	bool show_decorations(bool decorations);
    bool set_fullscreen(bool fs);
	bool set_on_top(bool on_top);
	bool process_x11_events();
	bool resize_event(geometry_t geometry);
	bool swap_buffers();
	bool fetch_frames();
	bool display_frames();
	bool display_frames_impl(const std::vector<core::pFrame>& frames);
	bool redraw_display();
private:
	gl::GL					gl_;
	using display_deleter = std::function<void(Display*)>;
	std::string 			display_str_;
	std::unique_ptr<Display, display_deleter>
							display_;
	Window					root_;
	Window					win_;
	int						screen_number_;
	std::vector<GLint>		attributes_;
	geometry_t				geometry_;
	XVisualInfo*			visual_;
	GLXContext				glx_context_;


	bool					flip_x_;
	bool					flip_y_;
	bool					read_back_;
	stereo_mode_t			stereo_mode_;
	bool 					decorations_;
	bool					on_top_;
	bool                    fullscreen_;
	std::vector<core::pFrame>
							frames_;
	std::vector<core::pFrame>
							old_frames_;
	bool                    use_30bit_;
	core::pConvert			converter_;
	std::vector<format_t>	supported_formats_;
	bool 					swap_eyes_;
	float					delta_x_;
	float					delta_y_;
	bool 					needs_move_;
	bool					show_cursor_;
	bool					needs_redraw_;
	timestamp_t				counter_start_;
	size_t					counter_;
	Atom					wm_delete_window_;
};

} /* namespace glx_window */
} /* namespace yuri */
#endif /* GLXWINDOW_H_ */


