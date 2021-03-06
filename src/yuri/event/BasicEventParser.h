/*!
 * @file 		BasicEventParser.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		09.07.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef BASICEVENTPARSER_H_
#define BASICEVENTPARSER_H_
#include "BasicEventProducer.h"

namespace yuri {
namespace event {
namespace parser {
enum class 	token_type_t {
	invalid,
	route,
	brace,
	func_name,
	spec,
	int_const,
	double_const,
	string_const,
	bool_const,
	vector_const,
	dict_const,
	null_const,
	bang_const
};

enum class func_mode_t {
	evaluate_all,
	evaluate_first
};
using p_token = std::shared_ptr<struct token>;
struct token {
	token_type_t 				type;
								token(token_type_t tok):type(tok) {}
	virtual 					~token() {}
};
struct invalid_token: public token {
								invalid_token():token(token_type_t::invalid) {}

};
struct route_token: public token {
								route_token():token(token_type_t::route) {}
	p_token 					expr;
	std::vector<p_token> 		output;
};
struct spec_token: public token {
								spec_token(const std::string& node, const std::string& name)
									:token(token_type_t::spec),node(node),name(name) {}
	const std::string 			node;
	const std::string 			name;
	p_token						init;
};
struct func_token: public token {
								func_token(const std::string& fname)
									:token(token_type_t::func_name),fname(fname),mode(func_mode_t::evaluate_all) {}
	std::string 				fname;
	func_mode_t 				mode;
	std::vector<p_token> 		args;
};
struct bool_const_token: public token {
								bool_const_token(bool val)
									:token(token_type_t::bool_const),val(val) {}
	bool 						val;
};
struct int_const_token: public token {
								int_const_token(int64_t val)
									:token(token_type_t::int_const),val(val) {}
	int64_t 					val;
};
struct double_const_token: public token {
								double_const_token(long double val)
									:token(token_type_t::double_const),val(val) {}
	long double 				val;
};
struct string_const_token: public token {
								string_const_token(const std::string& val)
									:token(token_type_t::string_const),val(val) {}
	std::string 				val;
};
struct vector_const_token: public token {
								vector_const_token()
									:token(token_type_t::vector_const) {}
	std::vector<p_token> 		members;
};
struct dict_const_token: public token {
								dict_const_token()
									:token(token_type_t::dict_const) {}
	std::map<std::string, p_token>
								members;
};

struct null_const_token: public token {
								null_const_token()
									:token(token_type_t::null_const) {}
};
struct bang_const_token: public token {
								bang_const_token()
									:token(token_type_t::bang_const) {}
};

EXPORT bool 					is_simple_route(const p_token& ast);
EXPORT std::pair<std::vector<p_token>, std::string>
								parse_string(const std::string& text);



}
class EventRouter;
class BasicEventParser: public BasicEventProducer, public BasicEventConsumer {
public:
	EXPORT 						BasicEventParser(log::Log&);
	EXPORT virtual 				~BasicEventParser() {}
	EXPORT static pBasicEvent 	parse_expr(log::Log&, const std::string& text, const std::map<std::string, pBasicEvent>& inputs);
private:
	virtual pBasicEventProducer find_producer(const std::string& name) = 0;
	virtual pBasicEventConsumer find_consumer(const std::string& name) = 0;

protected:
	EXPORT virtual bool 		do_process_event(const std::string& event_name, const event::pBasicEvent& event);
	EXPORT bool 				parse_routes(const std::string& text);
	EXPORT pBasicEvent 			parse_const(const std::string& text);

	EXPORT bool 				run_routers();
private:
	std::vector<std::shared_ptr<EventRouter>>
								routers_;
	log::Log&					log_pa_;

};

}
}

#endif /* BASICEVENTPARSER_H_ */
