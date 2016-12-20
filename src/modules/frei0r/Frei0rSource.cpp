/*!
 * @file 		Frei0rSource.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		05.06.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "Frei0rSource.h"
#include "yuri/core/Module.h"

#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/utils/DirectoryBrowser.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/utils/global_time.h"
#include <iostream>

namespace yuri {
namespace frei0r {


IOTHREAD_GENERATOR(Frei0rSource)

core::Parameters Frei0rSource::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("Frei0rSource");
	p["resolution"]["Source resolution"]=resolution_t{800,600};
	p["fps"]["Framerate limit. Set to -1 to unlimited"]=-1;
	return p;
}

Frei0rSource::Frei0rSource(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
IOThread(log_,parent,0,1,std::string("frei0r")),Frei0rBase(log, parameters)
{
	IOTHREAD_INIT(parameters)
	module_ = make_unique<frei0r_module_t>(path_);

	log[log::info] << "Opened module " << module_->info.name << " by " << module_->info.author;
	log[log::info] << "type: " << module_->info.plugin_type << ", color: " << module_->info.color_model;
	switch (module_->info.color_model) {
		case F0R_COLOR_MODEL_BGRA8888:
			format_ = core::raw_format::bgra32;
			break;
		case F0R_COLOR_MODEL_RGBA8888:
			format_ = core::raw_format::rgba32;
			break;
		case F0R_COLOR_MODEL_PACKED32:
		default:
			throw exception::InitializationFailed("Unsupported color format");
	}


}

Frei0rSource::~Frei0rSource() noexcept
{
	if (instance_) {
		module_->destruct(instance_);
	}
}

void Frei0rSource::run()
{
	if (!instance_) {
		instance_ = module_->construct(resolution_.width, resolution_.height);
		set_frei0r_params();
	}
	timestamp_t next_time;
	const duration_t time_delta = fps_ > 0.0f ? 1_s / fps_ : 0_s;
	while (still_running()) {
		if (fps_ > 0.0) {
			const timestamp_t now;
			if (now < next_time) {
				const auto nap = std::min(get_latency(), next_time - now);
				sleep(nap);
				continue;
			}
			next_time = next_time + time_delta;
		}
		auto outframe = core::RawVideoFrame::create_empty(format_, resolution_);
		auto dur = timestamp_t{} - core::utils::get_global_start_time();
		module_->update(instance_, dur.value/1.0e6, nullptr, reinterpret_cast<uint32_t*>(PLANE_RAW_DATA(outframe,0)));
		push_frame(0, std::move(outframe));
	}
}

bool Frei0rSource::set_param(const core::Parameter& param)
{
	if (Frei0rBase::set_param(param)) {
		return true;
	}
	if (assign_parameters(param)
			(resolution_, "resolution")
			(fps_, "fps"))
		return true;
	return core::IOThread::set_param(param);
}

} /* namespace frei0r */
} /* namespace yuri */
