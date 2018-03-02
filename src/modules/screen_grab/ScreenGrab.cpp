/*
 * ScreenGrab.cpp
 *
 *  Created on: 7.3.2013
 *      Author: neneko
 */
#include "ScreenGrab.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include <X11/extensions/Xfixes.h>
#include "X11/Xutil.h"
#include "X11/Xatom.h"
#include <string>
namespace yuri {
namespace screen {


IOTHREAD_GENERATOR(ScreenGrab)

MODULE_REGISTRATION_BEGIN("screen")
		REGISTER_IOTHREAD("screen",ScreenGrab)
MODULE_REGISTRATION_END()

core::Parameters ScreenGrab::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("ScreenGrab module.");
	p["display"]["X display"]=std::string();
	p["fps"]["Frames per second (set to 0 or negative, to grab at max. speed)"]=0.0;
	p["position"]["Position of grabbed area"]="0x0";
	p["resolution"]["Resolution of the grabbed image (set to 0x0 to grab full image)"]="0x0";
	p["cursor"]["Grabbing cursor as well"]=false;
	p["win_name"]["Window name (set to empty string to grab whole screen)"]=std::string();
	p["pid"]["PID of application that created the window (set to 0 to grab whole screen)"]=0;
	p["win_id"]["Window ID (set to 0 to grab whole screen)"]=0;
	return p;
}
namespace {
struct DisplayDeleter{
	void operator()(Display*d) { XCloseDisplay(d); }
};
struct ImageDeleter{
	void operator()(XImage*i) { XDestroyImage(i); }
};
std::string get_win_name(Display* dpy, Window win)
{
	std::string str;
	char *win_name;
	XFetchName(dpy, win, &win_name);
	if (win_name) {
		str = win_name;
		XFree(win_name);
	}
	return str;
}
size_t get_win_pid(Display* dpy, Window win)
{
	Atom atom = XInternAtom(dpy, "_NET_WM_PID", True);
	if (atom==None) return 0;
	Atom           type;
	int            format;
	unsigned long  nItems;
	unsigned long  bytesAfter;
	unsigned char *pid_ = 0;
	size_t 	pid = 0;
	if(XGetWindowProperty(dpy, win, atom, 0, 1, False, XA_CARDINAL, &type, &format, &nItems, &bytesAfter, &pid_)==Success)
	{
		if(pid_ != 0)
		{
			pid = *reinterpret_cast<unsigned long *>(pid_);
			XFree(pid_);
		}
	}
	return pid;
}
// Dummy method just to be able to use find)child bellow
Window get_win_id(Display* /*dpy*/, Window win) {
	return win;
}
template<typename T, typename F>
Window find_child(Display* dpy, Window top, T val, F func)
{
	if (func(dpy, top)==val) return top;
	Window dummy_win;
	Window *childs;
	Window found_win = 0;
	unsigned int child_count;
	XQueryTree(dpy,top,&dummy_win,&dummy_win,&childs,&child_count);
	if (childs && child_count) {
		for (unsigned int i=0; i<child_count;++i) {
			if((found_win = find_child(dpy,childs[i],val,func))!=0) break;
		}
	}
	XFree(childs);
	return found_win;
}
}

ScreenGrab::ScreenGrab(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,1,1,std::string("screen_grab")),fps_(0.0), win(0),position_{0,0},
resolution_{0,0},cursor_(false),pid(0),win_id_(0)
{
	IOTHREAD_INIT(parameters)
	XInitThreads();
	dpy.reset(XOpenDisplay(display.c_str()),DisplayDeleter());
	if (!dpy) {
		throw exception::InitializationFailed("Failed to open connection to X display at '"+display+"'");
	}
	log[log::info] << "Connected to display " << display;
	win = DefaultRootWindow(dpy.get());
	if (win_id_ != 0) {
		win = find_child(dpy.get(),win,win_id_,get_win_id);
		if (!win) {
			throw exception::InitializationFailed("Failed to find window with specified ID");
		}
	} else if (!win_name.empty()) {
		win = find_child(dpy.get(),win,win_name,get_win_name);
		if (!win) {
			throw exception::InitializationFailed("Failed to find window "+win_name);
		}
	} else if (pid) {
		win = find_child(dpy.get(),win,pid,get_win_pid);
		if (!win) {
			throw exception::InitializationFailed("Failed to find window for specified PID");
		}
	}
	log[log::info] << "Grabbing window " << get_win_name(dpy.get(), win);

}

ScreenGrab::~ScreenGrab() noexcept
{
}


namespace {
int error_handler(Display *, XErrorEvent *)
{
	throw std::runtime_error("Gwahahaha");
}
}
void ScreenGrab::run()
{
	XSetErrorHandler(error_handler);
	timestamp_t last_time_;
	const duration_t delta = fps_>0.0?1_s/fps_:0_s;
	while(still_running()) {
		if (fps_ > 0.0) {
			timestamp_t tnow;
			const auto tdelta = tnow - last_time_;
			if (tdelta < delta) {
				sleep((delta-tdelta)/2.0);
				continue;
			}
			last_time_ += delta;
		}
		step();
	}
	close_pipes();
	XSetErrorHandler(nullptr);
	dpy.reset();
}

bool ScreenGrab::step()
{
	try {
		XWindowAttributes attr;
		XGetWindowAttributes(dpy.get(),win,&attr);
		log[log::debug] << "Found window " << attr.width << "x" << attr.height;
		if (position_.x > attr.width || position_.y > attr.height) {
			log[log::warning] << "Offset out of range of the image";
			return true;
		}
		const int w = (resolution_.width>0)?std::min<int>(attr.width-position_.x,resolution_.width):attr.width-position_.x;
		const int h = (resolution_.height>0)?std::min<int>(attr.height-position_.y,resolution_.height):attr.height-position_.y;

		std::shared_ptr<XImage> img (XGetImage(dpy.get(),win,position_.x,position_.y,w,h,AllPlanes,ZPixmap),ImageDeleter());
		if (!img) {
			log[log::warning] << "Failed to get image from the window";
			return true;
		}
		log[log::debug] << "Image has depth " << img->depth << ", bpl: " << img->bytes_per_line << ", bpp: " << img->bits_per_pixel;
		format_t fmt = 0;
		switch (img->bits_per_pixel) {
			case 32: fmt = core::raw_format::bgra32; break;
			case 24: fmt = core::raw_format::bgr24; break;
		}

		if (fmt!=0) {
			resolution_t res = {static_cast<dimension_t>(w), static_cast<dimension_t>(h)};
			core::pRawVideoFrame frame = core::RawVideoFrame::create_empty(fmt, res, true);
			uint8_t* data = reinterpret_cast<uint8_t*>(img->data);
			if (cursor_) {
				XFixesCursorImage *xcim = XFixesGetCursorImage(dpy.get());
				const int x = xcim->x - xcim->xhot;
				const int y = xcim->y - xcim->yhot;
				const int to_line = std::min<int>((y + xcim->height), (h + position_.y));
				const int to_column = std::min<int>((x + xcim->width), (w + position_.x));
				for (int line = std::max<int>(y, position_.y); line < to_line; line++) {
					for (int column = std::max<int>(x, position_.x); column < to_column; column++) {
						int xcim_addr = (line - y) * xcim->width + column - x;
						int image_addr = ((line - position_.y) * w + column - position_.x) * img->bits_per_pixel/8;
						int r = (uint8_t)(xcim->pixels[xcim_addr] >>  0);
						int g = (uint8_t)(xcim->pixels[xcim_addr] >>  8);
						int b = (uint8_t)(xcim->pixels[xcim_addr] >> 16);
						int a = (uint8_t)(xcim->pixels[xcim_addr] >> 24);
						if (a == 255) {
							data[image_addr+0] = r;
							data[image_addr+1] = g;
							data[image_addr+2] = b;
						} else if (a) {
							data[image_addr+0] = r + (data[image_addr+0]*(255-a) + 255/2) / 255;
							data[image_addr+1] = g + (data[image_addr+1]*(255-a) + 255/2) / 255;
							data[image_addr+2] = b + (data[image_addr+2]*(255-a) + 255/2) / 255;
						}
					}
				}
			}
			uint8_t *out = PLANE_RAW_DATA(frame,0);
			const size_t copy_bytes = w*img->bits_per_pixel/8;
			for (int line=0;line<h;++line) {
				std::copy(data+img->bytes_per_line*line,data+img->bytes_per_line*line+copy_bytes,out);
				out+=copy_bytes;
			}
			push_frame(0,frame);
		}
		return true;
	} catch(std::runtime_error&){
		log[log::error] << "Failed to grab window! It's probably not mapped...";
	}
	return true;
}

bool ScreenGrab::set_param(const core::Parameter &param)
{
	if (assign_parameters(param)
			(display, "display")
			(fps_, "fps")
			(position_, "position")
			(resolution_, "resolution")
			(cursor_, "cursor")
			(win_name, "win_name")
			(pid, "pid")
			(win_id_, "win_id"))
		return true;

	return core::IOThread::set_param(param);
}

} /* namespace screen */
} /* namespace yuri */


