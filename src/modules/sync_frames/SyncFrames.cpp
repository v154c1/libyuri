/*!
 * @file 		SyncFrames.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		23.03.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "SyncFrames.h"
#include "yuri/core/Module.h"

namespace yuri {
namespace sync_frames {


IOTHREAD_GENERATOR(SyncFrames)

MODULE_REGISTRATION_BEGIN("sync_frames")
		REGISTER_IOTHREAD("sync_frames",SyncFrames)
MODULE_REGISTRATION_END()

core::Parameters SyncFrames::configure()
{
	core::Parameters p = base_type ::configure();
	p.set_description("SyncFrames");
	p["tolerance"]["Max timestamp difference for frames to be still considered the same (in ms)."]=5;
	p["main_input"]=-2;
	return p;
}


SyncFrames::SyncFrames(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
base_type(log_,parent,2, 2, std::string("sync_frames")),
tolerance_(5_ms)
{
	IOTHREAD_INIT(parameters)

	log[log::info] << "Using tolerance " << tolerance_;
}

SyncFrames::~SyncFrames() noexcept
{
}

std::vector<core::pFrame> SyncFrames::do_single_step(std::vector<core::pFrame> frames)
{
	bool ok = true;
	if (frames.empty()) return {};
	if (frames.size() == 1) return frames;
	const auto base = frames[0]->get_timestamp();
	for (const auto& f: frames)
	{
		const auto diff = abs(base - f->get_timestamp());
		if (diff > tolerance_) {
			ok = false;
			log[log::info] << "Timestamp difference " << diff << " is larger than tolerance " << tolerance_;
			break;
		}
	}
	if (ok) return frames;
	return {};
}

bool SyncFrames::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			.parsed<int64_t>
			(tolerance_, "tolerance", [](int64_t v){return 1_ms * v;}))
		return true;
	return base_type ::set_param(param);
}

} /* namespace sync_frames */
} /* namespace yuri */
