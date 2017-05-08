/*!
 * @file 		Convert.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		30.10.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef CONVERT_H_
#define CONVERT_H_

#include "yuri/core/thread/IOFilter.h"
#include "yuri/core/thread/ConverterThread.h"
namespace yuri {
namespace core {
class Convert;
typedef std::shared_ptr<Convert> pConvert;
class Convert: public core::IOFilter, public core::ConverterThread
{
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	Convert(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~Convert() noexcept;

	pFrame convert_to_any(pFrame frame, const std::vector<format_t>& fmts);
	pFrame convert_to_cheapest(pFrame frame, const std::vector<format_t>& fmts);

private:
	pFrame 	do_convert_frame(pFrame frame_in, format_t target_format);
	pFrame 	do_simple_single_step(pFrame frame);
	virtual bool set_param(const core::Parameter& param);
	format_t	format_;
	bool allow_passthrough_;
	size_t threads_;

	struct convert_pimpl_;
	std::unique_ptr<convert_pimpl_> pimpl_;
};

} /* namespace convert */
} /* namespace yuri */
#endif /* CONVERT_H_ */
