/*!
 * @file 		SpecializedIOFilter.h
 * @author 		Zdenek Travnicek
 * @date 		2.10.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SPECIALIZEDIOFILTER_H_
#define SPECIALIZEDIOFILTER_H_

#include "IOFilter.h"
namespace yuri {
namespace core {
template<class FrameType>
class SpecializedIOFilter: public IOFilter
{
public:
	using frame_type = FrameType;
	using p_frame_type = std::shared_ptr<frame_type>;
	SpecializedIOFilter(const log::Log &log_, pwThreadBase parent, const std::string& id = "SpecialFilter"):
		IOFilter(log_, parent, id){}
	virtual 				~SpecializedIOFilter() noexcept {}
private:
	virtual pFrame			do_simple_single_step(pFrame frame) override
	{
		auto sframe = std::dynamic_pointer_cast<frame_type>(std::move(frame));
		frame.reset();
		if (!sframe) return {};
		return do_special_single_step(std::move(sframe));
	}
	virtual pFrame			do_special_single_step(p_frame_type frame) = 0;
};

}
}

#endif /* SPECIALIZEDIOFILTER_H_ */
