/*!
 * @file 		SpecializedIOFilter.cpp
 * @author 		Zdenek Travnicek
 * @date 		2.10.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SPECIALIZEDMULTIIOFILTER_H_
#define SPECIALIZEDMULTIIOFILTER_H_
#include "MultiIOFilter.h"
#include <tuple>
#include <cassert>
namespace yuri {
namespace core {


template<typename Iter>
std::tuple<> verify_types(Iter)
{
	return {};
}

template<class FrameType, class... Others, typename Iter>
std::tuple<std::shared_ptr<FrameType>, std::shared_ptr<Others>...>
verify_types(Iter frame_iter)
{
	auto frame = std::dynamic_pointer_cast<FrameType>(std::move(*frame_iter));
	if (!frame) throw std::runtime_error("Wrong type");
	return std::tuple_cat(std::make_tuple(frame), verify_types<Others...>(frame_iter+1));
}



template<class... InFrameTypes>
class SpecializedMultiIOFilter: public MultiIOFilter {
public:
	static const size_t input_frames_count = sizeof...(InFrameTypes);
	using param_type = std::tuple<std::shared_ptr<InFrameTypes>... >;

	SpecializedMultiIOFilter(const log::Log &log_, pwThreadBase parent, position_t out_p,
			const std::string& id = "Spec. Multi Filter")
	:MultiIOFilter(log_, parent, input_frames_count, out_p, id) {}

	~SpecializedMultiIOFilter() noexcept {}
private:
	virtual std::vector<pFrame> do_single_step(std::vector<pFrame> frames) override final {
		assert (frames.size() == input_frames_count);
		try {
//			Timer t;
			auto p = verify_types<InFrameTypes...>(frames.begin());
//			auto t1 = t.get_duration();
			frames.clear();
			auto f = do_special_step(std::move(p));
//			auto t2 = t.get_duration();
//			log[log::info] << "Processing took "<<t2<<" in total, conversions " << t1 << " and module took " << (t2-t1);
			return f;
//			return do_special_step();
		}
		catch (std::exception& e) {
			log[log::info] << "Wrong frame types specified: " << e.what();
			throw;
		}
	}

	virtual std::vector<pFrame> do_special_step(param_type frames) = 0;

};


}
}

#endif /* SPECIALIZEDMULTIIOFILTER_H_ */

