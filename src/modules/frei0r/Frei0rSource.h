/*!
 * @file 		Frei0rSource.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		05.06.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef FREI0RSOURCE_H_
#define FREI0RSOURCE_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "Frei0rBase.h"

namespace yuri {
namespace frei0r {

class Frei0rSource: public core::IOThread, private Frei0rBase
{
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	Frei0rSource(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~Frei0rSource() noexcept;
private:
	virtual void run() override;
	virtual bool set_param(const core::Parameter& param) override;

	resolution_t resolution_;
	format_t format_;
	float fps_;
};

} /* namespace frei0r */
} /* namespace yuri */
#endif /* FREI0RSOURCE_H_ */
