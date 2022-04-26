/*!
 * @file 		GlxWindow.cpp
 * @author 		<Your name>
 * @date		25.01.2015
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#include "GlxWindow.h"
#include "yuri/core/Module.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/thread/Convert.h"
#include "yuri/core/utils/assign_events.h"
#include <cstring>
#include <poll.h>
namespace yuri {
namespace glx_window {


IOTHREAD_GENERATOR(GlxWindow)

MODULE_REGISTRATION_BEGIN("glx_window")
		REGISTER_IOTHREAD("glx_window",GlxWindow)
MODULE_REGISTRATION_END()

core::Parameters GlxWindow::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("GlxWindow");
	p["stereo"]["Stereoscopic method (none, anaglyph, quadbuffer, side_by_side, top_bottom)"]="none";
	p["flip_x"]["Flip around vertical axis"]=false;
	p["flip_y"]["Flip around horizontal axis"]=false;
	p["read_back"]["Read drawn picture back and output it"]=false;
	p["resolution"]["Window resoluton"]=resolution_t{800,600};
	p["position"]["Window position"]=coordinates_t{0,0};
	p["decorations"]["Show window decorations"]=false;
	p["swap_eyes"]["Swap stereo eyes"]=false;
	p["delta_x"]["Horizontal correction (-1.0, 1.0)"]=0.0f;
	p["delta_y"]["Vertical correction (-1.0, 1.0)"]=0.0f;
	p["show_cursor"]["Enable or disable cursor in the window"]=true;
	p["on_top"]["Stay on top"]=false;
	p["fullscreen"]["Set window fullscreen"]=false;
	p["keys_autorepeat"]["Allows autorepeating hold keys."]=false;
	p["pbo"]["Use PBO to update display (larger latency, faster update"]=false;
	p["use_30bit"]["Use 30 bit colors"]=false;
	return p;
}


namespace {
void add_attribute(int attrib, std::vector<int>& attributes)
{
	if (!attributes.empty() && attributes[attributes.size()-1] == None) {
		attributes.pop_back();
	}
	attributes.push_back(attrib);
	attributes.push_back(None);
}

const std::map<std::string, stereo_mode_t> mode_names = {
		{"none", stereo_mode_t::none},
		{"quadbuffer", stereo_mode_t::quadbuffer},
		{"anaglyph", stereo_mode_t::anaglyph},
		{"side_by_side", stereo_mode_t::side_by_side},
		{"top_bottom", stereo_mode_t::top_bottom},
};

stereo_mode_t get_mode(const std::string& name) {
	auto it = mode_names.find(name);
	if (it == mode_names.end()) return stereo_mode_t::none;
	return it->second;
}

std::string get_mode_name(stereo_mode_t mode)
{
	for (const auto& m: mode_names) {
		if (mode == m.second) return m.first;
	}
	return "Unknown";
}
int stereo_frames_needed(stereo_mode_t mode) {
	if (mode == stereo_mode_t::none) return 1;
	return 2;
}
}

GlxWindow::GlxWindow(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,2,2,std::string("glx_window")),
BasicEventConsumer(log),
BasicEventProducer(log),
gl_(log),
display_str_{":0"},display_(nullptr,[](Display*d) { XCloseDisplay(d);}),
screen_number_{0},attributes_{GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None},
geometry_{800,600,0,0},visual_{nullptr},flip_x_{false},flip_y_{false},
read_back_{false},stereo_mode_{stereo_mode_t::none},decorations_{false},
on_top_{false},fullscreen_{false},use_30bit_{false},keys_autorepeat_{false},swap_eyes_{false},delta_x_{0.0},delta_y_{0.0},needs_move_{false},
show_cursor_{true},
counter_(0),
wm_delete_window_(0),
corners_(gl_.corners)
{
	set_latency(100_ms);
	IOTHREAD_INIT(parameters)
	if (stereo_mode_ == stereo_mode_t::quadbuffer) {
		add_attribute(GLX_STEREO, attributes_);
	}
	if(use_30bit_) {
        add_attribute(GLX_RED_SIZE, attributes_);
        add_attribute(10, attributes_);
        add_attribute(GLX_GREEN_SIZE, attributes_);
        add_attribute(10, attributes_);
        add_attribute(GLX_BLUE_SIZE, attributes_);
        add_attribute(10, attributes_);
	}
	if (!create_window()) {
		throw exception::InitializationFailed("Failed to create window");
	}

	supported_formats_ = gl_.get_supported_formats();
//	set_supported_formats(gl_.get_supported_formats());

}

GlxWindow::~GlxWindow() noexcept
{
}

void GlxWindow::run()
{
	if (!create_glx_context()) {
		log[log::warning] << "Failed to create GLX context";
		request_end(core::yuri_exit_interrupted);
	} else {
		show_decorations(decorations_);

		show_window();
		move_window({geometry_.x, geometry_.y});
		set_on_top(on_top_);
		if (fullscreen_) set_fullscreen();
		// Let's keep local converter until MultiIOThread supports this behaviour.
		converter_.reset(new core::Convert(log, get_this_ptr(), core::Convert::configure()));
		add_child(converter_);
		log[log::info] << "Stereo method: " << get_mode_name(stereo_mode_);
		show_cursor(show_cursor_);
	}
	wm_delete_window_ = XInternAtom(display_.get(), "WM_DELETE_WINDOW", false);
	XSetWMProtocols(display_.get(), win_, &wm_delete_window_, 1);
	while (still_running()) {
		process_events();
		if (needs_move_) {
			resize_window(geometry_.get_resolution());
			move_window({geometry_.x, geometry_.y});
			needs_move_ = false;
		}
		process_x11_events();
		wait_for(get_latency());
		if (display_frames()) {
			if (read_back_) {
				glReadBuffer(GL_BACK_LEFT);
				const auto res = geometry_.get_resolution();
				auto left = gl_.read_window(res.get_geometry());
				if (stereo_mode_ == stereo_mode_t::quadbuffer) {
					glReadBuffer(GL_BACK_RIGHT);
					auto right = gl_.read_window(res.get_geometry());
					push_frame(1, right);
				}
				push_frame(0, left);
			}
			swap_buffers();
		} else if (needs_redraw_) {
			if (redraw_display()) {
				swap_buffers();
			}
		}
		needs_redraw_ = false;
	}
}

bool GlxWindow::create_window()
{
	display_.reset(XOpenDisplay(display_str_.c_str()));
	if (!display_) return false;
	log[log::info] << "Connected to display " << display_str_;
	std::string::size_type ind=display_str_.find_last_of(':');
	if (ind!=std::string::npos) {
		ind = display_str_.find_first_of('.',ind);
		if (ind != std::string::npos) {
			screen_number_ = std::stoi(display_str_.substr(ind+1).c_str());
		}
	}
	log[log::info] << "Screen number is " << screen_number_;
	root_=RootWindow(display_.get(),screen_number_);
	if (!root_) return false;
	log[log::info] << "Found root window";
	visual_ = glXChooseVisual(display_.get(), screen_number_, &attributes_[0]);
	if (!visual_) return false;
	log[log::info] << "Found visual " << visual_->visualid;
	auto cmap = XCreateColormap(display_.get(), root_, visual_->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.override_redirect = true;
	swa.colormap = cmap;
	swa.event_mask =  ExposureMask   | KeyPressMask    | StructureNotifyMask
					| KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
					| PointerMotionMask ;//ResizeRedirectMask;
	swa.border_pixel = 0;
	swa.background_pixel = 0;
	log[log::info] << "geometry " << geometry_;
	win_ = XCreateWindow(display_.get(),
						 root_,
						 geometry_.x,
						 geometry_.y,
						 geometry_.width,
						 geometry_.height,
						 0,
						 visual_->depth,
						 InputOutput,
						 visual_->visual,
						 CWBackPixel | CWBorderPixel |CWColormap | CWEventMask,
						 &swa);
	log[log::info] << "X Window Created";
	return true;
}

bool GlxWindow::create_glx_context()
{
	//XStoreName(display_.get(), win_, winname.c_str());
//	yuri::lock_t bgl(GL::big_gpu_lock);
	glx_context_ = glXCreateContext(display_.get(), visual_, 0, GL_TRUE);
	if (!glx_context_) return false;
	glXMakeCurrent(display_.get(), win_, glx_context_);
	log[log::info] << "Created GLX Context";
//	bgl.unlock();
//	log[log::debug] << "Cursor " << (show_cursor ? "will" : "won't") << " be shown";
	return true;
}

bool GlxWindow::show_window(bool /* show */)
{
	XMapWindow(display_.get(), win_);
	return true;
}
bool GlxWindow::show_cursor(bool show)
{
	if (show) {
		XUndefineCursor(display_.get(), win_);
	} else {
		Pixmap pixmap;
		Cursor cursor;
		XColor color;
		// Create 1x1 1bpp pixmap
		pixmap = XCreatePixmap(display_.get(), win_, 1, 1, 1);
		std::fill_n(reinterpret_cast<char*>(&color),sizeof(XColor),0);
		cursor = XCreatePixmapCursor(display_.get(), pixmap, pixmap, &color, &color,
				0, 0);
		XDefineCursor(display_.get(), win_, cursor);
	}
	return true;
}
void GlxWindow::move_window(coordinates_t coord)
{
	XMoveWindow(display_.get(), win_, coord.x,coord.y);
	XRaiseWindow(display_.get(),win_);
}
void GlxWindow::resize_window(resolution_t res)
{
	XResizeWindow(display_.get(), win_, res.width,res.height);
	XRaiseWindow(display_.get(),win_);
}

bool GlxWindow::process_x11_events()
{
	XEvent event_, next_event_;
	bool processed_any = false;
	// auto x11_fd = ConnectionNumber(display_.get());
	// pollfd fds = {x11_fd, POLLIN, 0};
	// if (poll(&fds, 1, 0) <= 0) return false;
	while (XPending(display_.get())) {
//	while (XCheckWindowEvent(display_.get(),
//				win_,
//				StructureNotifyMask|KeyPressMask|KeyReleaseMask|
//				ButtonPressMask|ButtonReleaseMask|PointerMotionMask|
//				ExposureMask,
//				&event_))
//		{
//			fds.revents = 0;
			XNextEvent(display_.get(), &event_);
			switch (event_.type) {
			case DestroyNotify:
				log[log::info] << "DestroyNotify received";
				request_end(core::yuri_exit_interrupted);
				//parent->stop();
				break;
			case ConfigureNotify:
				resize_event(geometry_t{static_cast<dimension_t>(event_.xconfigure.width),
										static_cast<dimension_t>(event_.xconfigure.height),
										event_.xconfigure.x,
										event_.xconfigure.y});
				needs_redraw_ = true;
				break;
			case KeyPress:
//				emit_event("key"+std::to_string(event_.xkey.keycode));
				emit_event("key"+std::to_string(event_.xkey.keycode), true);
				emit_event("key_down",event_.xkey.keycode);
//				log[log::debug] << "Key " << do_get_keyname(event_.xkey.keycode) << " (" <<
//				event_.xkey.keycode << ") pressed" <<std::endl;
//				keys[event_.xkey.keycode]=true;
				// TODO: need to reenable this again!
				// std::string keyname = XKeysymToString(XkbKeycodeToKeysym(display.get(), event_.xkey.keycode, 0, 0));
				//if (keyCallback) keyCallback->run(&xev.xkey.keycode);
				if (event_.xkey.keycode==9) request_end(core::yuri_exit_interrupted);
				break;
			case KeyRelease:
				// Prevent autorepeat
				if (!keys_autorepeat_ && XEventsQueued(display_.get(), QueuedAfterReading)) {
					XPeekEvent(display_.get(), &next_event_);
					if (next_event_.type == KeyPress && event_.xkey.keycode == next_event_.xkey.keycode && event_.xkey.time == next_event_.xkey.time) {
						XNextEvent(display_.get(), &event_);
						break;
					}
				}
				emit_event("key"+std::to_string(event_.xkey.keycode), false);
				emit_event("key_up",event_.xkey.keycode);
				break;
			case ButtonPress:
				emit_event("button",event_.xbutton.button);
				emit_event("button"+std::to_string(event_.xbutton.button), true);
				break;
			case ButtonRelease:
				emit_event("button"+std::to_string(event_.xbutton.button), false);
				break;
			case MotionNotify: {
				std::vector<event::pBasicEvent> motion{
					std::make_shared<event::EventInt>(event_.xmotion.x, 0, geometry_.width),
					std::make_shared<event::EventInt>(event_.xmotion.y, 0, geometry_.height)};

				emit_event("mouse", std::make_shared<event::EventVector>(motion));
				emit_event("mouse_x",event_.xmotion.x, 0, geometry_.width);
				emit_event("mouse_y",event_.xmotion.y, 0, geometry_.height);
				} break;
			case Expose:
				needs_redraw_ = true;
				break;
			case ClientMessage:
				if (static_cast<Atom>(event_.xclient.data.l[0]) == wm_delete_window_) {
					log[log::info] << "Windows close message";
					request_end(core::yuri_exit_interrupted);
				}
				break;
			}
			processed_any = true;
		}
		return processed_any;

}
namespace {
typedef struct
{
    unsigned long   flags;
    unsigned long   functions;
    unsigned long   decorations;
    long            input_mode;
    unsigned long   status;
} wm_hints;
}
bool GlxWindow::show_decorations(bool decorations)
{
	wm_hints hints;
	Atom mh = None;
	mh=XInternAtom(display_.get(),"_MOTIF_WM_HINTS",0);
	hints.flags = 2;//MWM_HINTS_DECORATIONS;
	hints.decorations = decorations?1:0;
	hints.functions = 0;
	hints.input_mode = 0;
	hints.status = 0;
	int r = XChangeProperty(display_.get(), win_, mh, mh, 32, PropModeReplace,
			(unsigned char *) &hints, 5);
	log[log::info] << "Decorations XChangeProperty returned " << r;
	return true;
}
bool GlxWindow::set_fullscreen()
{
	auto wm_state = XInternAtom(display_.get(),"_NET_WM_STATE", 1);
	auto wm_state_fs = XInternAtom(display_.get(),"_NET_WM_STATE_FULLSCREEN", 1);
	int r = XChangeProperty(display_.get(), win_, wm_state, wm_state, 32, PropModeReplace,
			(unsigned char *) &wm_state_fs, 1);
	log[log::info] << "Fullscreen XChangeProperty returned " << r;
	XClearWindow(display_.get(), win_);
	XMapRaised(display_.get(), win_);
	XEvent event;
	event.type = ClientMessage;
	event.xclient.window = win_;
	event.xclient.message_type = wm_state;
	event.xclient.format = 32;
	event.xclient.data.l[0] = 1;
	event.xclient.data.l[1] = wm_state_fs;
	event.xclient.data.l[3] = 0l;
	XSendEvent(display_.get(), root_, 0, SubstructureNotifyMask, &event);
	return true;
}

bool GlxWindow::set_on_top(bool on_top)
{
//	wm_hints hints;

	Atom nwsa = None;
	nwsa=XInternAtom(display_.get(),"_NET_WM_STATE_ABOVE",1);
	if (nwsa == None) {
		log[log::warning] << "Display doesn't support _NET_WM_STATE_ABOVE property";
		return false;
	}
	Atom nws = None;
	nws=XInternAtom(display_.get(),"_NET_WM_STATE",1);
	if (nws == None) {
		log[log::warning] << "Display doesn't support _NET_WM_STATE property";
		return false;
	}
	XClientMessageEvent xclient;
	std::memset( &xclient, 0, sizeof (xclient) );

	xclient.type = ClientMessage;
	xclient.window = win_;
	xclient.message_type = nws;
	xclient.format = 32;
	xclient.data.l[0] = on_top?1:0;
	xclient.data.l[1] = nwsa;
	xclient.data.l[2] = 0;
	xclient.data.l[3] = 0;
	xclient.data.l[4] = 0;
	XSendEvent( display_.get(),
	  root_,  False,
	  SubstructureRedirectMask | SubstructureNotifyMask,
	  (XEvent *)&xclient );
	log[log::info] << "setting on top: " << on_top;
	return true;
}

bool GlxWindow::resize_event(geometry_t geometry)
{
	glViewport(0, 0, geometry.width, geometry.height);
	geometry_ = geometry;
	return true;
}

bool GlxWindow::swap_buffers()
{
	glXSwapBuffers(display_.get(), win_);
	return true;
}

namespace {
// Swaps 0 and 1
	inline int swapped_value(bool swap_needed, int i) {
		return swap_needed?1-i:i;
	}
}
bool GlxWindow::fetch_frames()
{
	// This should depend on policy
	auto needed = stereo_frames_needed(stereo_mode_);
	frames_.resize(needed);
	const bool swap_needed = swap_eyes_ && (needed == 2);
	for (auto i: irange(0, needed)) {
		if (!frames_[i]) {
			frames_[i] = converter_->convert_to_cheapest(
							pop_frame(
									swapped_value(swap_needed, i)
							), supported_formats_);
		}
	}
	for (const auto& f: frames_) {
		if (!f) return false;
	}
	return true;
}

namespace {
void draw_part(gl::GL& gl_, int i, core::pFrame frame, bool fx, bool fy, const std::array<float, 8>& corners)
{
	gl_.corners = corners;
	gl_.generate_texture(i, frame, fx, fy);
	gl_.draw_texture(i);
	gl_.finish_frame();

}
}

bool GlxWindow::display_frames()
{
	if (!fetch_frames()) return false;
	display_frames_impl(frames_);

	using std::swap;
	swap(old_frames_,frames_);
	frames_.clear();
	return true;
}

bool GlxWindow::redraw_display()
{
	if (static_cast<int>(old_frames_.size()) < stereo_frames_needed(stereo_mode_))
	{
		return false;
	}
	return display_frames_impl(old_frames_);
}

bool GlxWindow::display_frames_impl(const std::vector<core::pFrame>& frames)
{
	timestamp_t now{};
	auto delta = now - counter_start_;
	++counter_;
	if (delta > 5_s) {
		auto fps = 1000.0 * 1_s * counter_ / delta / 1000.0;
		log[log::info] << "FPS: " << fps;
		counter_ = 0;
		counter_start_ = now;
	}

	glDrawBuffer(GL_BACK_LEFT);
	gl_.clear();
	gl_.set_texture_delta(0,  delta_x_,  delta_y_);
	gl_.set_texture_delta(1, -delta_x_, -delta_y_);
    const auto avg = [&](size_t a, size_t b){return (corners_[a]+corners_[b])/2.0f;};
	switch(stereo_mode_) {
		case stereo_mode_t::none:
			draw_part(gl_, 0, frames[0], flip_x_, flip_y_, corners_);
			break;
		case stereo_mode_t::quadbuffer:
			{
				draw_part(gl_, 0, frames_[0], flip_x_, flip_y_, corners_);
				glDrawBuffer(GL_BACK_RIGHT);
				gl_.clear();
				draw_part(gl_, 1, frames[1], flip_x_, flip_y_, corners_);
			}; break;
		case stereo_mode_t::anaglyph:
				glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
				draw_part(gl_, 0, frames[0], flip_x_, flip_y_, corners_);
				glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_FALSE);
				draw_part(gl_, 1, frames[1], flip_x_, flip_y_, corners_);
				break;

		case stereo_mode_t::side_by_side:
			{
                auto corners1 = corners_;
                corners1[2] = avg(0, 2);
                corners1[3] = avg(1, 3);
                corners1[4] = avg(4, 6);
                corners1[5] = avg(5, 7);
				draw_part(gl_, 0, frames[0], flip_x_, flip_y_, corners1);
                auto corners2 = corners_;
                corners2[0] = avg(0, 2);
                corners2[1] = avg(1, 3);
                corners2[6] = avg(4, 6);
                corners2[7] = avg(5, 7);
				draw_part(gl_, 1, frames[1], flip_x_, flip_y_,  corners2);
			}; break;
		case stereo_mode_t::top_bottom:
			{
                auto corners1 = corners_;
                corners1[4] = avg(2, 4);
                corners1[5] = avg(3, 5);
                corners1[6] = avg(0, 6);
                corners1[7] = avg(1, 7);
				draw_part(gl_, 0, frames[0], flip_x_, flip_y_, corners1);
                auto corners2 = corners_;
                corners2[2] = avg(2, 4);
                corners2[3] = avg(3, 5);
                corners2[0] = avg(0, 6);
                corners2[1] = avg(1, 7);
				draw_part(gl_, 1, frames[1], flip_x_, flip_y_,  corners2);
			}; break;
		default:break;
	}
	return true;
}

bool GlxWindow::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			(flip_x_, "flip_x")
			(flip_y_, "flip_y")
			(read_back_, "read_back")
			(decorations_, "decorations")
			(on_top_, "on_top")
			(fullscreen_, "fullscreen")
			(keys_autorepeat_, "keys_autorepeat")
			(swap_eyes_, "swap_eyes")
			(delta_x_, "delta_x")
			(delta_y_, "delta_y")
			(display_str_, "display")
			(show_cursor_, "show_cursor")
			(gl_.use_pbo, "pbo")
            (use_30bit_, "use_30bit")
            (fullscreen_, "fullscreen")
			(stereo_mode_, "stereo", [](const core::Parameter& p){return get_mode(p.get<std::string>());}))
		return true;

	if (param.get_name() == "resolution") {
		auto res = param.get<resolution_t>();
		geometry_.width = res.width; geometry_.height = res.height;
	} else if (param.get_name() == "position") {
		auto pos = param.get<coordinates_t>();
		geometry_.x = pos.x; geometry_.y = pos.y;
		log[log::info] << "Geometry " << geometry_;
	} else return core::IOThread::set_param(param);
	return true;
}

bool GlxWindow::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	if (assign_events(event_name, event)
			(delta_x_, "dx", "delta_x")
			(delta_y_, "dy", "delta_y")
            .vector_values("corners", corners_[0], corners_[1], corners_[2], corners_[3], corners_[4], corners_[5], corners_[6], corners_[7])
            .vector_values("corner0", corners_[0], corners_[1])
            .vector_values("corner1", corners_[2], corners_[3])
            .vector_values("corner2", corners_[4], corners_[5])
            .vector_values("corner3", corners_[6], corners_[7])
                    (corners_[0], "x0")
                    (corners_[1], "y0")
                    (corners_[2], "x1")
                    (corners_[3], "y1")
                    (corners_[4], "x2")
                    (corners_[5], "y2")
                    (corners_[6], "x3")
                    (corners_[7], "y3"))
		return true;

	if (assign_events(event_name, event)
			(geometry_.x, "x")
			(geometry_.y, "y")
			(geometry_.width, "width")
			(geometry_.height, "height")) {
		needs_move_ = true;
		return true;
	}

	return false;
}
} /* namespace glx_window */
} /* namespace yuri */
