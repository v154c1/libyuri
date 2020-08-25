/*!
 * @file 		RealSense.h
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		25.08.2020
 * @copyright	Institute of Intermedia, CTU in Prague, 2020
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef REALSENSE_H_
#define REALSENSE_H_

#include "yuri/core/thread/IOThread.h"
#include <librealsense2/rs.hpp>

namespace yuri {
namespace realsense {

class RealSense: public core::IOThread
{
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	RealSense(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~RealSense() noexcept;
private:
	
	virtual void run() override;
	virtual bool set_param(const core::Parameter& param) override;

    core::pRawVideoFrame remove_bg(const rs2::frame& color_frame, const rs2::frame& depth_frame);

private:
    rs2::pipeline pipeline_;
    rs2::pipeline_profile profile_;
    float depth_scale_ = -1.0;
    float clip_distance_ = -1.0;
};

} /* namespace realsense */
} /* namespace yuri */
#endif /* REALSENSE_H_ */
