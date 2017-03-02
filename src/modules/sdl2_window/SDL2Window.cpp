/*!
 * @file 		SDL2Window.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		06.08.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "SDL2Window.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/utils/irange.h"
//#include "yuri/core/utils/environment.h"

#include "yuri/version.h"
#include <unordered_map>
#include <numeric>

namespace yuri {
namespace sdl2_window {


IOTHREAD_GENERATOR(SDL2Window)

MODULE_REGISTRATION_BEGIN("sdl2_window")
		REGISTER_IOTHREAD("sdl2_window",SDL2Window)
MODULE_REGISTRATION_END()

core::Parameters SDL2Window::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("SDL2Window");
	p["resolution"]["Window resolution"]=resolution_t{800, 600};
	p["position"]["Window position"]=coordinates_t{0,0};
	p["decorations"]["Show windoww borders"]=true;
	p["title"]["Window title"]=yuri_version;
	p["resizable"]["Make the window resizable"]=true;
	p["fullscreen"]["Open the window at full screen"]=false;
	p["keep_aspect"]["Keep image aspect ration"]=true;
	p["screen"]["Screen number. Setting to -1 will force autodetection"]=-1;
	p["background_color"]["Color for background when the image is letterboxed"]=core::color_t::create_rgb(0,0,0);
	return p;
}

namespace {

mutex sdl_global_mutex;

bool sdl_initialized = false;
mutex sdl_initialization_mutex;
bool sdl_init_video() {
	lock_t _(sdl_initialization_mutex);
	if (sdl_initialized) return true;
	return sdl_initialized = (SDL_Init(SDL_INIT_VIDEO) == 0);
}
using namespace yuri::core::raw_format;
const std::unordered_map<format_t, Uint32> yuri_to_sdl_formats = {
		{yuyv422, SDL_PIXELFORMAT_YUY2},
		{uyvy422, SDL_PIXELFORMAT_UYVY},
		{yvyu422, SDL_PIXELFORMAT_YVYU},

		{rgb24, SDL_PIXELFORMAT_RGB24},
		{bgr24, SDL_PIXELFORMAT_BGR24},
#if 1 // Little endian
		{abgr32, SDL_PIXELFORMAT_RGBA8888},
		{argb32, SDL_PIXELFORMAT_BGRA8888},
		{bgra32, SDL_PIXELFORMAT_ARGB8888},
		{rgba32, SDL_PIXELFORMAT_ABGR8888},
#endif
		{rgb15, SDL_PIXELFORMAT_RGB555},
		{bgr15, SDL_PIXELFORMAT_BGR555},
		{rgb16, SDL_PIXELFORMAT_RGB565},
		{bgr16, SDL_PIXELFORMAT_BGR565},
		{rgb8,  SDL_PIXELFORMAT_RGB332},
};

Uint32 get_sdl_format(format_t fmt)
{
	auto it = yuri_to_sdl_formats.find(fmt);
	if (it == yuri_to_sdl_formats.end()) return SDL_PIXELFORMAT_UNKNOWN;
	return it->second;
}

}


SDL2Window::SDL2Window(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
base_type(log_,parent,2,0,std::string("sdl2_window")),
window_resolution_{800,600},
window_position_{0,0},
window_title_{yuri_version},
window_decorations_{true},
window_resizable_{true},
window_fullscreen_{false},
window_keep_aspect_{true},
window_(nullptr,[](SDL_Window* w){if(w)SDL_DestroyWindow(w);}),
renderer_(nullptr,[](SDL_Renderer* w){if(w)SDL_DestroyRenderer(w);}),
texture_(nullptr,[](SDL_Texture* w){if(w)SDL_DestroyTexture(w);}),

last_texture_format_(0)
//screen_(nullptr)
{
	IOTHREAD_INIT(parameters)
	if (!sdl_init_video()) {
		throw exception::InitializationFailed("Failed to initialize SDL2");
	}


    const auto displays = SDL_GetNumVideoDisplays();

    if (displays < 1) {
    	log[log::info] << "No displays supported?";
    } else {
    	SDL_Rect r;
    	for (auto i: irange(displays)) {
			if (SDL_GetDisplayBounds(i, &r) < 0) {
				log[log::info] << "Failed to query info for SDL display " << i ;
			} else {
				auto g = geometry_t{static_cast<dimension_t>(r.w),static_cast<dimension_t>(r.h),r.x, r.y};
				log[log::info] << "SDL supported display " << i << " has geometry " << g;
			}
    	}
    }

    if (screen_number_ >= 0) {
    	log[log::error] << "Screen handling is horribly broken in SDL2, don't expect it to work reliably.";
    }


	for(const auto& f: yuri_to_sdl_formats) {
		supported_formats_.push_back(f.first);
	}

}



SDL2Window::~SDL2Window() noexcept
{
}

bool SDL2Window::process_sdl_events()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				request_end(core::yuri_exit_interrupted);
				return false;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					request_end(core::yuri_exit_interrupted);
					return false;
				} else if (event.key.keysym.sym == 'f') {
					window_fullscreen_ = !window_fullscreen_;
					SDL_SetWindowFullscreen(window_.get(), window_fullscreen_?SDL_WINDOW_FULLSCREEN_DESKTOP:0);
				} break;
			case SDL_WINDOWEVENT:
//				log[log::info] << "Window event";
				switch (event.window.event) {
					case SDL_WINDOWEVENT_CLOSE:
						log[log::info] << "Window CLOSE";
						request_end(core::yuri_exit_interrupted);
						return false;
				} break;
		}
	}
	return true;
}

namespace {

template<class Texture>
void update_texture(const Texture& texture_, const core::pRawVideoFrame& f)
{
	uint8_t* pixels = nullptr;
	int pitch = 0;
	const auto res  = f->get_resolution();
	lock_t _(sdl_global_mutex);
	SDL_LockTexture(texture_.get(),
					nullptr,
					reinterpret_cast<void**>(&pixels),
					&pitch);

	const auto data = PLANE_RAW_DATA(f, 0);
	const int linesize = (*f)[0].get_line_size();
	const auto copy_bytes = std::min(linesize, pitch);
	for (auto line: irange(res.height)) {
		const auto line_start = data + linesize*line;
		std::copy(line_start, line_start + copy_bytes, pixels + pitch*line);
	}

	SDL_UnlockTexture(texture_.get());
}

template<class Win>
SDL_Renderer* create_renderer_try_flags(const Win& window, const std::vector<SDL_RendererFlags>& f0)
{
	Uint32 flags = std::accumulate(f0.begin(), f0.end(), Uint32{}, [](Uint32 a, Uint32 b){return a|b;});
	if (auto ret = SDL_CreateRenderer(window.get(),
							-1,
							flags)) {
		return ret;
	}
	if (f0.empty()) return nullptr;
	return create_renderer_try_flags(window, std::vector<SDL_RendererFlags>(f0.begin()+1, f0.end()));
}


}




bool SDL2Window::create_renderer()
{
	lock_t _(sdl_global_mutex);
	Uint32 flags = SDL_WINDOW_ALLOW_HIGHDPI |
			(window_resizable_?SDL_WINDOW_RESIZABLE:0) |
			(window_decorations_?0:SDL_WINDOW_BORDERLESS) |
			(window_fullscreen_?SDL_WINDOW_FULLSCREEN_DESKTOP:0);

	auto coord = window_position_;
	if (screen_number_ >= 0) {
		coord.x = coord.y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(screen_number_);
	}
	//core::utils::set_environmental_variable("DISPLAY",":0.2",1);


	window_.reset(SDL_CreateWindow(window_title_.c_str(),
									  coord.x,
									  coord.y,
									  window_resolution_.width,
									  window_resolution_.height,
									  flags));
	if (!window_) return false;

	renderer_.reset(create_renderer_try_flags(window_, {SDL_RENDERER_PRESENTVSYNC, SDL_RENDERER_PRESENTVSYNC}));

	SDL_RendererInfo rinfo;
	SDL_GetRendererInfo(renderer_.get(), &rinfo);
	log[log::info] << "Initialized 2D renderer " << rinfo.name << ", " << (rinfo.flags&SDL_RENDERER_ACCELERATED?"with":"without") << " HW acceleration";
	return true;
}
bool SDL2Window::verify_texture_format(const core::pRawVideoFrame& frame)
{
	const auto res  = frame->get_resolution();
	const auto format = frame->get_format();
	if (!texture_ || format != last_texture_format_ || res != last_texture_res_) {
		const auto sdl_format = get_sdl_format(format);
		if (!sdl_format) {
			log[log::warning] << "Unsupported format " << core::raw_format::get_format_name(format);
			return false;
		}
		lock_t _(sdl_global_mutex);
		texture_.reset(SDL_CreateTexture(renderer_.get(),
										sdl_format,
										SDL_TEXTUREACCESS_STREAMING,
										res.width,
										res.height));
		if (!texture_) {
			log[log::warning] << "Failed to create texture for foramt " << core::raw_format::get_format_name(format);
			return false;
		}
		log[log::info] << "Generated texture in format " << core::raw_format::get_format_name(format);
		last_texture_res_ = res;
		last_texture_format_ = format;
	}
	return true;
}

void SDL2Window::run()
{
	converter_ = std::make_shared<core::Convert>(log, get_this_ptr(), core::Convert::configure());

	create_renderer();

	SDL_RenderPresent(renderer_.get());
	while(still_running()) {
		wait_for(get_latency());
		process_sdl_events();


		auto frame = std::dynamic_pointer_cast<core::RawVideoFrame>(converter_->convert_to_cheapest(pop_frame(0), supported_formats_));
		if (!frame) continue;

		if (!verify_texture_format(frame))
			continue;

		update_texture(texture_, frame);
		SDL_SetRenderDrawColor(renderer_.get(),
						background_color_.r(),
						background_color_.g(),
						background_color_.b(),
						background_color_.a());

		SDL_RenderClear(renderer_.get());
		SDL_Rect rect = {0, 0, 0, 0};


		SDL_GetRendererOutputSize(renderer_.get(), &rect.w, &rect.h);

		if (window_keep_aspect_) {
			const auto res  = frame->get_resolution();
			const auto ar_renderer = static_cast<double>(rect.w)/rect.h;
			const auto ar_frame= static_cast<double>(res.width)/res.height;
			if (ar_renderer > ar_frame) {
				auto w = static_cast<int>(ar_frame * rect.h);
				rect.x = (rect.w - w) / 2;
				rect.w = w;
			} else {
				auto h = static_cast<int>(rect.w / ar_frame);
				rect.y = (rect.h - h) / 2;
				rect.h = h;
			}
		}

		SDL_RenderCopy(renderer_.get(), texture_.get(), nullptr, &rect);
		SDL_RenderPresent(renderer_.get());
	}

	texture_.reset();
	renderer_.reset();
	window_.reset();
}

bool SDL2Window::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			(window_resolution_,	"resolution")
			(window_position_,		"position")
			(window_decorations_,	"decorations")
			(window_resizable_,		"resizable")
			(window_title_,			"title")
			(window_fullscreen_,	"fullscreen")
			(window_keep_aspect_,	"keep_aspect")
			(screen_number_,		"screen")
			(background_color_,		"background_color"))
		return true;

	return core::IOThread::set_param(param);
}

} /* namespace sdl2_window */
} /* namespace yuri */
