/*!
 * @file 		XmlBuilder.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		15.9.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef XMLBUILDER_H_
#define XMLBUILDER_H_

#include "GenericBuilder.h"
#include "yuri/event/BasicEventParser.h"
namespace yuri {
namespace core {

struct variable_info_t {
	std::string name;
	std::string description;
	std::string value;
};

class XmlBuilder: public GenericBuilder
{
public:
	EXPORT static Parameters configure();
	IOTHREAD_GENERATOR_DECLARATION
	EXPORT XmlBuilder(const log::Log& log_, pwThreadBase parent, const Parameters& parameters);
	EXPORT XmlBuilder(const log::Log& log_, pwThreadBase parent, const std::string& filename, const std::vector<std::string>& argv, bool parse_only = false);
	EXPORT ~XmlBuilder() noexcept;

	EXPORT const std::string& get_app_name();
	EXPORT const std::string& get_description();
	EXPORT std::vector<variable_info_t> get_variables() const;
private:
	EXPORT virtual bool set_param(const Parameter& parameter) override;

	EXPORT virtual void run() override;
	EXPORT virtual bool step() override;
	struct builder_pimpl_t;
	std::unique_ptr<builder_pimpl_t>	pimpl_;
	std::string	filename_;
	duration_t max_run_time_;
	timestamp_t start_time_;
	bool use_variable_events_;
};
}
}



#endif /* XMLBUILDER_H_ */
