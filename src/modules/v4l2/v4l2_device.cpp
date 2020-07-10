/*!
 * @file 		v4l2_device.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		01.03.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "v4l2_device.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if YURI_BSD
#include <sys/videoio.h>
#else
#include <linux/videodev2.h>
#include <linux/version.h>
#endif
#include <sys/ioctl.h>
#include "yuri/core/utils/DirectoryBrowser.h"
#include "yuri/core/utils.h"
#include "yuri/core/utils/irange.h"

#include <sys/mman.h>
#include <poll.h>

// for memalign
#include <stdlib.h>

#include <cstring>

namespace yuri {
namespace v4l2 {

namespace {
	int xioctl(int fd, unsigned long int request, void *arg)
	{
		int r;
		while((r = ::ioctl (fd, request, arg)) < 0) {
			if (r==-1 && errno==EINTR) continue;
			break;
		}
		return r;
	}

	std::vector<std::pair<size_t, size_t>> known_resolutions = {
        {640,  360 },
        {800,  600 },
		{1024, 768 },
		{1280, 720 },
		{1280, 800 },
		{1280, 1024},
		{1600, 900 },
		{1920, 1080},
		{1920, 1200},
		{2048, 1152},
		{2560, 1440},
		{3840, 2160}
	};

	std::vector<std::pair<size_t, size_t>> known_fps = {
        {1, 90},
        {1, 60},
		{1, 50},
		{1, 30},
		{1, 25},
		{1, 15},
		{1, 10},
		{1,  5},
		{1,  1}
	};
}

std::vector<std::string> enum_v4l2_devices()
{
	auto paths  = core::filesystem::browse_files("/sys/class/video4linux");
	std::vector<std::string> out;
	for (const auto& p: paths) {
		auto idx = p.find_last_of("/");
		out.push_back("/dev"+p.substr(idx));
	}
	return out;
}


//v4l2_device::v4l2_device():fd_(0)
//{
//}

v4l2_device::v4l2_device(const std::string& path)
:method_(capture_method_t::none),imagesize_(0),running_(false)
{
	fd_ = ::open(path.c_str(),O_RDWR|O_NONBLOCK);
	if (fd_ < 0) throw std::runtime_error("Failed to open file " + path);

	v4l2_capability cap;
	if (xioctl(fd_,VIDIOC_QUERYCAP,&cap)<0) {
		throw std::runtime_error("Failed to get device info");
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		throw std::runtime_error("Device doesn't support video capture");
	}
	std::string ver;
	ver = lexical_cast<std::string>((cap.version >> 16) & 0xFF)  + "."
			+ lexical_cast<std::string>((cap.version >> 8) & 0xFF ) + "."
			+ lexical_cast<std::string>(cap.version & 0xFF);
	uint8_t dev_caps = (cap.device_caps & V4L2_CAP_STREAMING?streaming:0) |
						(cap.device_caps & V4L2_CAP_READWRITE?read_write:0);
	info_ = v4l2_device_info{reinterpret_cast<char*>(cap.card),
							reinterpret_cast<char*>(cap.driver),
							ver,
							reinterpret_cast<char*>(cap.bus_info),
							dev_caps};
}

v4l2_device::v4l2_device(v4l2_device&& rhs) noexcept
:fd_(rhs.fd_),method_(rhs.method_),imagesize_(0),running_(rhs.running_)
{
	rhs.fd_ = 0;
	rhs.running_ = false;
}
v4l2_device& v4l2_device::operator=(v4l2_device&& rhs) noexcept
{
	fd_ = rhs.fd_;
	method_ = rhs.method_;
	running_ = rhs.running_;
	rhs.fd_ = 0;
	rhs.running_ = false;
	return *this;
}
v4l2_device::~v4l2_device() noexcept
{
	if (fd_>0) {
		::close(fd_);
	}
}

int v4l2_device::get() const
{
	return fd_;
}

v4l2_device_info v4l2_device::get_info()
{
	return info_;
}


std::vector<v4l2_input_info> v4l2_device::enum_inputs()
{
	v4l2_input input_info;
	input_info.index=0;
	std::vector<v4l2_input_info> inputs;
	while (xioctl(fd_,VIDIOC_ENUMINPUT,&input_info) == 0) {
		v4l2_input_info info;
		info.name = reinterpret_cast<char*>(input_info.name);
		if (input_info.type!=V4L2_INPUT_TYPE_CAMERA)
		{
			info.state = not_camera;
		}
		if (input_info.status==V4L2_IN_ST_NO_POWER) info.state |= no_power;
		if (input_info.status==V4L2_IN_ST_NO_SIGNAL) info.state |= no_signal;
		if (input_info.status==V4L2_IN_ST_NO_COLOR) info.state |= no_color;
		if (input_info.status==V4L2_IN_ST_HFLIP) info.state |= horiz_flip;
		if (input_info.status==V4L2_IN_ST_VFLIP) info.state |= vert_flip;
		inputs.push_back(info);
		input_info.index++;
	}
	return inputs;
}

bool v4l2_device::set_input(int index)
{
	return xioctl (fd_, VIDIOC_S_INPUT, &index) == 0;
}

std::vector<v4l2_std_info> v4l2_device::enum_standards()
{
	v4l2_standard std_info;
	std_info.index=0;
	std::vector<v4l2_std_info> stds;
	while (xioctl(fd_,VIDIOC_ENUMSTD,&std_info) == 0) {
		v4l2_std_info info;
		info.name = reinterpret_cast<char*>(std_info.name);
		info.id = std_info.id;
		stds.push_back(info);
	}
	return stds;
}

std::vector<uint32_t> v4l2_device::enum_formats()
{
	v4l2_fmtdesc fmts;
	std::vector<uint32_t> supp_formats;
	fmts.index=0;
	fmts.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (xioctl(fd_,VIDIOC_ENUM_FMT,&fmts) == 0) {
		fmts.index++;
		supp_formats.push_back(fmts.pixelformat);
	}
	return supp_formats;
}

std::vector<resolution_t> v4l2_device::enum_resolutions(uint32_t fmt)
{
	v4l2_frmsizeenum frms;
	frms.index=0;
	frms.pixel_format = fmt;
	std::vector<resolution_t> res;
	while (xioctl(fd_,VIDIOC_ENUM_FRAMESIZES,&frms) == 0) {
		switch (frms.type) {
			case V4L2_FRMSIZE_TYPE_DISCRETE:
				//l << "discrete " << resolution_t{frms.discrete.width, frms.discrete.height};
				res.push_back({frms.discrete.width, frms.discrete.height});
				break;
			case V4L2_FRMSIZE_TYPE_CONTINUOUS:
//				l << "continuous";
				break;
			case V4L2_FRMSIZE_TYPE_STEPWISE:
				for (size_t height = frms.stepwise.min_height; height < frms.stepwise.max_height; height+=frms.stepwise.step_height) {
					for (size_t width = frms.stepwise.min_width; width < frms.stepwise.max_width; width+=frms.stepwise.step_width) {
						for (auto& k_res : known_resolutions)
							if (k_res.first == width && k_res.second == height) res.push_back({width, height});
					}
				}
//				l << "stepwise";
				break;
		}
		frms.index++;
	}
	return res;
}

std::vector<fraction_t> v4l2_device::enum_fps(uint32_t fmt, resolution_t res)
{
	v4l2_frmivalenum frmvalen;
	std::vector<fraction_t> fps_list;
	frmvalen.pixel_format = fmt;
	frmvalen.width = res.width;
	frmvalen.height = res.height;
	frmvalen.index = 0;
	while (xioctl(fd_,VIDIOC_ENUM_FRAMEINTERVALS,&frmvalen) == 0) {
		switch (frmvalen.type) {
		case V4L2_FRMIVAL_TYPE_CONTINUOUS:
//			log[log::info] << "Supports continuous frame_intervals from"
//				<< frmvalen.stepwise.min.numerator << "/" << frmvalen.stepwise.min.denominator
//				<< "s to " << frmvalen.stepwise.max.numerator << "/" << frmvalen.stepwise.max.denominator <<"s"<< std::endl;
			for (size_t numerator = frmvalen.stepwise.min.numerator; numerator <= frmvalen.stepwise.max.numerator; numerator++) {
				for (size_t denominator = frmvalen.stepwise.max.denominator; denominator <= frmvalen.stepwise.min.denominator; denominator++) {
					for (auto& k_fps : known_fps)
						if (k_fps.first == numerator && k_fps.second == denominator) fps_list.push_back({numerator, denominator});
				}
			}
			break;
		case V4L2_FRMIVAL_TYPE_STEPWISE:
//			log[log::info] << "Supports stepwise frame_intervals from"
//				<< frmvalen.stepwise.min.numerator << "/" << frmvalen.stepwise.min.denominator
//				<< "s to " << frmvalen.stepwise.max.numerator << "/" << frmvalen.stepwise.max.denominator
//				<< "s with step " << frmvalen.stepwise.step.numerator << "/" << frmvalen.stepwise.step.denominator<<"s"<< std::endl;
				for (size_t numerator = frmvalen.stepwise.min.numerator; numerator <= frmvalen.stepwise.max.numerator; numerator+=frmvalen.stepwise.step.numerator) {
					for (size_t denominator = frmvalen.stepwise.max.denominator; denominator <= frmvalen.stepwise.min.denominator; denominator+=frmvalen.stepwise.step.denominator) {
						for (auto& k_fps : known_fps)
							if (k_fps.first == numerator && k_fps.second == denominator) fps_list.push_back({numerator, denominator});
					}
				}
			break;
		case V4L2_FRMIVAL_TYPE_DISCRETE:
//			if (!frmvalen.index) log[log::info] << "Supports discrete frame_intervals:" << std::endl;
//			log[log::info] << "\t"<<frmvalen.index<<": "<< frmvalen.discrete.numerator << "/" << frmvalen.discrete.denominator << "s" << std::endl;
			fps_list.push_back({frmvalen.discrete.denominator, frmvalen.discrete.numerator});
			break;
		}
		frmvalen.index++;
//		if (!frmvalen.index) break;
		//log[log::info] << "Supported frame_interval " << fmts.index << ": " << fmts.description << std::endl;

		//supported_formats.push_back(fmts.pixelformat);
	}
	return fps_list;
}

bool v4l2_device::set_default_cropping()
{
	v4l2_cropcap cropcap;
	v4l2_crop crop;
	std::memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (!xioctl (fd_, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */
//		log[log::info] << "Selected input have pixel with aspect ratio " <<
//				cropcap.pixelaspect.numerator << "/" << cropcap.pixelaspect.denominator << std::endl;
		if (xioctl (fd_, VIDIOC_S_CROP, &crop) == -1) {
//			log[log::warning] <<"VIDIOC_S_CROP failed, ignoring :)" << std::endl;
			return false;
		}
	} else {
		return false;
//		log[log::warning] << "Failed to query cropping info, ignoring" << std::endl;
	}
	return true;
}

v4l2_format_info v4l2_device::set_format(uint32_t format, resolution_t resolution)
{
	v4l2_format fmt;
	std::memset (&fmt,0,sizeof(v4l2_format));
	v4l2_format_info out_info;
	fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (xioctl(fd_,VIDIOC_G_FMT,&fmt)<0) {
//		log[log::error] << "VIDIOC_G_FMT ioctl failed! (" << strerror(errno)
//							<< ")" << std::endl;
//		return false;
		throw std::runtime_error("Failed to get default format info!");
	}
	fmt.fmt.pix.pixelformat=format;
	fmt.fmt.pix.width=resolution.width;
	fmt.fmt.pix.height=resolution.height;
	if (xioctl(fd_,VIDIOC_S_FMT,&fmt)<0) {
//		log[log::error] << "VIDIOC_S_FMT ioctl failed!";
		throw std::runtime_error ("Failed to set input format!");
//		return false;
	}
	if (xioctl(fd_,VIDIOC_G_FMT,&fmt)<0) {
//		log[log::warning] << "Failed to verify if input format was set correctly !";
	}
	if (fmt.fmt.pix.pixelformat != format) {
//		log[log::error] << "Failed to set requested input format!";
		throw std::runtime_error("Failed to set input format");
//		return false;
	}
//	log[log::info] << "Video dimensions: " << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << std::endl;
//	log[log::info] << "Pixel format (" << fmt.fmt.pix.pixelformat << "): " <<
//		(char)(fmt.fmt.pix.pixelformat & 0xFF) <<
//		(char)(fmt.fmt.pix.pixelformat>>8 & 0xFF) <<
//		(char)(fmt.fmt.pix.pixelformat>>16 & 0xFF) <<
//		(char)(fmt.fmt.pix.pixelformat>>24 & 0xFF)
//		<< std::endl;
//	log[log::info] << "Colorspace " << fmt.fmt.pix.colorspace;
	out_info.imagesize=fmt.fmt.pix.sizeimage;
	out_info.resolution = resolution_t{fmt.fmt.pix.width, fmt.fmt.pix.height};
	return out_info;
//	pixelformat=fmt.fmt.pix.pixelformat;

//	log[log::info] << "Image size: " << imagesize << std::endl;
//	return true;
}

fraction_t v4l2_device::set_fps(fraction_t fps)
{
	v4l2_streamparm strp;
	strp.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	strp.parm.capture.capability=V4L2_CAP_TIMEPERFRAME;
	strp.parm.capture.readbuffers=0;
	strp.parm.capture.extendedmode=0;
	strp.parm.capture.capturemode=0;
	// Numerator and denominator are switched because we're changing it from FPS to frame time...
	strp.parm.capture.timeperframe.numerator=fps.denom;
	strp.parm.capture.timeperframe.denominator=fps.num;
	if (xioctl(fd_,VIDIOC_S_PARM,&strp)<0) {
//		log[log::error] << "Failed to set frame parameters (FPS)";
		//throw exception::Exception ("Failed to set input format!");
		return {0, 0};
	}
	if (xioctl(fd_,VIDIOC_G_PARM,&strp)<0) {
//		log[log::error] << "Failed to verify frame parameters (FPS)";
		//throw exception::Exception ("Failed to set input format!");
		return {0, 0};
	}
	fps = {strp.parm.capture.timeperframe.denominator,strp.parm.capture.timeperframe.numerator};
	return fps;
//	log[log::info] << "Driver reports current frame interval " << !fps << "s";
//	if (fps.valid() || fps.num!=0) {
//		frame_duration = 1_s/fps.get_value();
//	} else {
//		frame_duration = 0_s;
//	}
//	return true;
}

std::vector<buffer_t> v4l2_device::init_mmap()
{
	struct v4l2_requestbuffers req;
	std::memset(&req, 0, sizeof(req));
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (xioctl (fd_, VIDIOC_REQBUFS, &req)<0) {
		if (errno == EINVAL) {
//			log[log::warning] << "Device does not support memory mapping";
	            return {};
		} else {
//			log[log::warning] << "VIDIOC_REQBUFS failed";
			return {};
		}
	}
	if (req.count < 2) {
//		log[log::warning] << "Insufficient buffer memory";
		return {};
	}
	auto no_buffers=req.count;
	std::vector<buffer_t> buffers (no_buffers);

	for (auto i: irange(0, req.count)) {
		v4l2_buffer buf;
		std::memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    buf.memory = V4L2_MEMORY_MMAP;
	    buf.index = i;
	    if (xioctl (fd_, VIDIOC_QUERYBUF, &buf)<0) {
//	    	log[log::error] << "VIDIOC_QUERYBUF failed. (" << strerror(errno)
	    	return {};
	    }
	    auto ptr = ::mmap (nullptr, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buf.m.offset);
		if (ptr == MAP_FAILED) {
			return {};
		}
		auto len = buf.length;
		buffers[i].data.set(reinterpret_cast<uint8_t*>(ptr), buf.length, [ptr,len](void*)noexcept{::munmap(ptr, len);});
	}
	return buffers;
}

std::vector<buffer_t> v4l2_device::init_user(size_t imagesize)
{
	struct v4l2_requestbuffers req;
	unsigned long buffer_size=imagesize;
	auto page_size = ::getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	std::memset(&req,0,sizeof(req));

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	if (xioctl (fd_, VIDIOC_REQBUFS, &req)<0) {
		if (errno == EINVAL) {
			return {};
		}
	}
	if (req.count < 2) {
//		log[log::warning] << "Insufficient buffer memory";
		return {};
	}
	auto no_buffers=req.count;
	std::vector<buffer_t> buffers (no_buffers);
	for (auto i: irange(0, req.count)) {
		void *ptr = nullptr;
		auto ret = ::posix_memalign (&ptr, page_size, buffer_size);
		if (ret != 0 || !ptr) {
			return {};
		}
		buffers[i].data.set(reinterpret_cast<uint8_t*>(ptr), buffer_size, [ptr, buffer_size](void*)noexcept{::free(ptr);});
	}
	return buffers;
}

std::vector<buffer_t> v4l2_device::init_read(size_t imagesize)
{
	std::vector<buffer_t> buffers (1);
	buffers[0].data.resize(imagesize);
	return buffers;
}

bool v4l2_device::initialize_capture(size_t imagesize, capture_method_t method, log::Log& log)
{
	if (method == capture_method_t::mmap && !(info_.caps & streaming) ) {
		log[log::error] << "Requested MMAP to access camera, but the camera doesn't support it.";
		return false;
	}
	if (method == capture_method_t::user && !(info_.caps & streaming) ) {
		log[log::error] << "Requested USER pointers to access camera, but the camera doesn't support it.";
		return false;
	}
	if (method == capture_method_t::read && !(info_.caps & read_write)) {
		log[log::error] << "Requested direct read from camera, but the camera doesn't support it.";
		return false;
	}
	if ((info_.caps & streaming) && (method != capture_method_t::read)) {
		log[log::debug] << "Driver supports streaming operations, trying to initialize";
		if (method == capture_method_t::none || method == capture_method_t::mmap) {
			log[log::info] << "Initializing mmap";
			buffers_ = init_mmap();
			if (!buffers_.empty()) {
				log[log::info] << "Initialized capture using mmap";
				method_ = capture_method_t::mmap;
				imagesize_ = imagesize;
				return true;
			}
		}
		if (method == capture_method_t::none || method == capture_method_t::user) {
			log[log::debug] << "Initializing user pointers";
			buffers_ = init_user(imagesize);
			if (!buffers_.empty()) {
				log[log::info] << "Initialized capture using user pointers";
				method_ = capture_method_t::user;
				imagesize_ = imagesize;
				return true;
			}
		}
	}
	if((info_.caps & read_write)) {
		log[log::debug] << "Driver supports read/write operations";
		if (method == capture_method_t::none || method == capture_method_t::read) {
			buffers_ = init_read(imagesize);
			if (!buffers_.empty()) {
				log[log::info] << "Initialized direct reading from device file";
				method_=capture_method_t::read;
				imagesize_ = imagesize;
				return true;
			}
		}
	}
	return false;
}

bool v4l2_device::start_capture()
{
	enum v4l2_buf_type type;
	switch (method_) {
		case capture_method_t::read:
			running_ = true;
			return true;
		case capture_method_t::mmap:
			for (auto i: irange(0, buffers_.size())) {
//				if (buffers_[i].start == MAP_FAILED) {
//						log[log::error] << "mmap failed (" << errno << ") - "
//							<< strerror(errno) << std::endl;
//						return false;
//				}
				struct v4l2_buffer buf;
				std::memset (&buf, 0, sizeof(buf));
				buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory      = V4L2_MEMORY_MMAP;
				buf.index       = i;
				if (xioctl (fd_, VIDIOC_QBUF, &buf) == -1) {
//					log[log::error] << "VIDIOC_QBUF failed (" << strerror(errno)
//							<< ")" << std::endl;
					return false;
				}
			}
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (xioctl(fd_, VIDIOC_STREAMON, &type)==-1) {
//				log[log::error] << "VIDIOC_STREAMON failed (" << strerror(errno)
//										<< ")" << std::endl;
								return false;
			}
			running_ = true;
			return true;

		case capture_method_t::user:
			for (auto i: irange(0, buffers_.size())) {
				struct v4l2_buffer buf;
				std::memset (&buf, 0, sizeof(buf));
				buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory      = V4L2_MEMORY_USERPTR;
				buf.index       = i;
				buf.m.userptr   = (unsigned long) buffers_[i].data.data();
				buf.length      = buffers_[i].data.size();
				if (xioctl (fd_, VIDIOC_QBUF, &buf) == -1) {
//					log[log::error] << "VIDIOC_QBUF failed (" << strerror(errno)
//							<< ")" << std::endl;
					return false;
				}
				type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				if (xioctl (fd_, VIDIOC_STREAMON, &type)==-1) {
//					log[log::error] << "VIDIOC_STREAMON failed (" << strerror(errno)
//							<< ")" << std::endl;
					return false;
				}
			}
			running_ = true;
			return true;
		case capture_method_t::none:
			return false;
		default: return false;
	}
}

bool v4l2_device::stop_capture()
{
	enum v4l2_buf_type type;
	switch (method_) {
		case capture_method_t::read:
			return true;
		case capture_method_t::mmap:
		case capture_method_t::user:
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (xioctl (fd_, VIDIOC_STREAMOFF, &type)==-1) {
//				log[log::error] << "VIDIOC_STREAMOFF failed";
				return false;
			}
			return true;
		case capture_method_t::none:
			return false;
		default:
			return false;
	}
}


bool v4l2_device::read_frame(std::function<bool(uint8_t*, size_t)> func)
{
	int res = 0;
	struct v4l2_buffer buf;
	switch (method_) {
		case capture_method_t::read:
			res = ::read(fd_, buffers_[0].data.data(), imagesize_);
			if (res < 0) {
				if (errno == EAGAIN || errno == EINTR) return true;
//						log[log::error] << "Read error (" << errno << ") - " << strerror(errno);
				return false;
			}
			if (!res)
				return false; // Should never happen
			return func(buffers_[0].data.data(), res);
		case capture_method_t::mmap:
			std::memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			if (xioctl (fd_, VIDIOC_DQBUF, &buf) == -1) {
				switch (errno) {
					case EAGAIN:
						return false;//{nullptr, 0};
					case EIO:
					default:
//									log[log::error] << "VIDIOC_DQBUF failed (" <<
//										strerror(errno) << ")";
						//start_capture();

						for (auto i: irange(0, buffers_.size())) {
							memset(&buf, 0, sizeof(buf));
							buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
							buf.memory = V4L2_MEMORY_MMAP;
							buf.index = i;
							if ( xioctl(fd_,VIDIOC_QUERYBUF,&buf) < 0) {
//								log[log::error] << "Failed to query buffer " << i << "(" << strerror(errno);
								continue;
							}
							if ((buf.flags & (V4L2_BUF_FLAG_QUEUED | V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_DONE)) == V4L2_BUF_FLAG_MAPPED) {
								if (xioctl(fd_,VIDIOC_QBUF,&buf)<0) {
//									log[log::error] << "Failed to queue buffer " << i << "(" << strerror(errno);
//									return false;
								}
							}
						}

						return false;
				}
			}
			if (buf.index >= buffers_.size()) {
//						log[log::error] << "buf.index >= n_buffers!!!!";
				return false;//{nullptr, 0};
			}

//					log[log::verbose_debug] << "Pushing frame with " << buf.bytesused
//							<< "bytes";
			{
				bool r = func?func(buffers_[buf.index].data.data(),buf.bytesused):false;

//					prepare_frame(reinterpret_cast<uint8_t*>(buffers[buf.index].start),buf.bytesused);
				if (xioctl (fd_, VIDIOC_QBUF, &buf) == -1) {
//							log[log::error] << "VIDIOC_QBUF failed";
					return false;
				}
				return r;
			}
//			break;
			//return true;
		case capture_method_t::user:
					std::memset(&buf, 0, sizeof(buf));
					buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buf.memory = V4L2_MEMORY_MMAP;
					if (xioctl (fd_, VIDIOC_DQBUF, &buf) == -1) {
						switch (errno) {
							case EAGAIN:
									return true;
							case EIO:
							default:
//									log[log::error] << "VIDIOC_DQBUF failed";
									return false;
							}
					}
//					prepare_frame(reinterpret_cast<uint8_t*>(buf.m.userptr), buf.length);
					func(buffers_[buf.index].data.data(),buf.bytesused);
					if (xioctl (fd_, VIDIOC_QBUF, &buf)==-1) {
//							log[log::error] << "VIDIOC_QBUF failed";
							return false;
					}
					break;
		case capture_method_t::none:
		default: return false;
	}
	return true;
}

bool v4l2_device::wait_for_data(duration_t duration)
{
	pollfd fds = {fd_, POLLIN, 0};
	::poll(&fds, 1, static_cast<int>(duration.value/1000));
	return (fds.revents & POLLIN);

}


/* ***********************************************************
 *  Controls
 *************************************************************/

control_state_t v4l2_device::is_control_supported(uint32_t id)
{
	struct v4l2_queryctrl queryctrl;
	std::memset (&queryctrl, 0, sizeof (queryctrl));
	queryctrl.id = id;

	if (xioctl (fd_, VIDIOC_QUERYCTRL, &queryctrl) < 0) {
//		log[log::debug]<< "Control " << queryctrl.name << " not supported";
		return {control_support_t::not_supported, 0, 0, 0, {}};
	} else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
//		log[log::debug]<< "Control " << queryctrl.name << " disabled";
		return {control_support_t::disabled, 0, 0, 0, {}};
	}
	return {control_support_t::supported, 0, queryctrl.minimum, queryctrl.maximum, reinterpret_cast<char*>(queryctrl.name)};
}

control_state_t v4l2_device::is_user_control_supported(uint32_t id)
{
	auto state = is_control_supported(id);
	if (state.supported == control_support_t::supported) {
		struct v4l2_control control;
		std::memset (&control, 0, sizeof (control));
		control.id = id;
		if (xioctl (fd_, VIDIOC_G_CTRL, &control)>=0) {
			state.value = control.value;
		}
	}
	return state;
}

bool v4l2_device::set_user_control(uint32_t id, control_state_t state, int32_t value)
{
	struct v4l2_control control;
	std::memset (&control, 0, sizeof (control));
	control.id = id;
	control.value = clip_value(value, state.min_value, state.max_value);

	if (xioctl (fd_, VIDIOC_S_CTRL, &control) < 0) {
//		log[log::debug]<< "Failed to enable control " << state.name;
		return false;

	} else {
		control.value = 0;
		if (xioctl (fd_, VIDIOC_G_CTRL, &control) >= 0) {
//			log[log::info] << "Control " << state.name << " set to " << control.value;
		} else {
//			log[log::warning] << "Failed to set value for " << state.name;
			return false;
		}
	}
	return true;
}

control_state_t v4l2_device::is_camera_control_supported(uint32_t id)
{
	auto state = is_control_supported(id);
	if (state.supported == control_support_t::supported) {
		v4l2_ext_control control {id, 0, {0}, {0}};
#if YURI_BSD ||  LINUX_VERSION_CODE < KERNEL_VERSION(4,20,0)
		v4l2_ext_controls controls {V4L2_CID_CAMERA_CLASS, 1, 0, {0, 0}, &control};
#else
        v4l2_ext_controls controls {V4L2_CID_CAMERA_CLASS, 1, 0, 0, 0, &control};
#endif
		if (xioctl (fd_, VIDIOC_G_EXT_CTRLS, &controls)>=0) {
			state.value = control.value;
		}
	}
	return state;
}


bool v4l2_device::set_camera_control(uint32_t id, control_state_t state, int32_t value)
{
	v4l2_ext_control control {id, 0, {0}, {clip_value(value, state.min_value, state.max_value)}};
#if YURI_BSD ||  LINUX_VERSION_CODE < KERNEL_VERSION(4,20,0)
	v4l2_ext_controls controls {V4L2_CID_CAMERA_CLASS, 1, 0, {0, 0}, &control};
#else
    v4l2_ext_controls controls {V4L2_CID_CAMERA_CLASS, 1, 0, 0, 0, &control};
#endif

	if (xioctl (fd_, VIDIOC_S_EXT_CTRLS, &controls) < 0) {
//		log[log::debug]<< "Failed to enable control " << state.name;
		return false;
//	} else {
//		control.value = 0;
	}
	return true;
}

}
}

