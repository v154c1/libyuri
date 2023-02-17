/*!
 * @file 		BasicEventParser.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		09.07.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "BasicEventParser.h"
#include "BasicEventConversions.h"
#include "yuri/core/utils/make_unique.h"
#include <iostream>
#include <algorithm>
#include <cassert>

namespace yuri {
namespace event {
namespace parser {
namespace {
	const std::string white_space(" \n\t");
	const std::string id_allowed_characters("_");
	const std::string route_lex("route");






	template<class Iterator>
	void trim_string(Iterator& first, const Iterator& last)
	{
		typedef typename std::iterator_traits<Iterator>::value_type iter_type;
		first = std::find_if(first, last,
				[](const iter_type& val){
					return std::none_of(white_space.begin(), white_space.end(),
							[&val](const char& c)
								{return c==val;}
					);});
	}
	/*!
	 * Looks up a substring at the beginning of interval <first1, last1)
	 *
	 * @param first1 Begining of the string to search
	 * @param last1 End of the string to search
	 * @param second string to find
	 * @return Returns iterator to first cahracter after the found substring (may be equal to last1).
	 * 		If the string was not found, it return first1
	 */
	template<class Iterator1, class Str>
	Iterator1 find_substr(Iterator1 first1, const Iterator1& last1, const Str& second)
	{
		if (static_cast<size_t>(std::distance(first1,last1)) < second.size()) return first1;
		auto orig_first1=first1;
		auto first2 = second.begin();
		const auto& last2 = second.end();
		while (first2!=last2) {
			if (*first2 != *first1) return orig_first1;
			++first2;
			++first1;
		}

		return first1;
	}

	template<class Iterator>
	std::string find_id(Iterator& first, const Iterator& last, bool allow_star = false)
	{
		std::string id {};
		bool first_lex = true;
		if (allow_star && *first == '*') {
			++first;
			return "*";
		}
		for (;first!=last;++first,first_lex=false) {
			const auto& ch = *first;
			if (ch >= 'a' && ch <= 'z') {id+=ch;continue;}
			if (ch >= 'A' && ch <= 'Z') {id+=ch;continue;}
			if (ch == '_') {id+=ch;continue;}
			if (ch == '/') {id+=ch;continue;}
			if (ch == '-') {id+=ch;continue;}
			// TODO: this is obviously broken...
			for (const auto& c: id_allowed_characters) { if (ch==c) {id+=ch;continue;}}
			if (!first_lex) {if (ch >= '0' && ch <= '9') {id+=ch;continue;}}
			break;
		}
		return id;
	}


	template<class Iterator>
	p_token parse_expr(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_spec(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_arg(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_func(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_bool(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_double_or_int(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_string(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_vector(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_dict(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_null(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_bang(Iterator& first, const Iterator& last);
	template<class Iterator>
	p_token parse_const(Iterator& first, const Iterator& last);




	template<class Iterator>
	p_token parse_spec(Iterator& first, const Iterator& last)
	{
		std::string node{}, name{};
		trim_string(first,last);
		if (first != last && *first == '@') {
			node = "@";
			++first;
			// We allow both @param and @:param
			if (first != last && *first==':') ++first;
		} else {
			node = find_id(first,last);
			if (node.empty()) return p_token();
			trim_string(first,last);
			if (first == last || *first != ':') return p_token();
			++first;
		}
		trim_string(first,last);
		name = find_id(first, last, true);
		if (name.empty()) return p_token();
		auto spec = std::make_shared<spec_token>(node,name);
		trim_string(first,last);
		if (first != last && *first == '|') {
			++first;
			spec->init = parse_arg(first,last);
		}
		return spec;
	}
	template<class Iterator>
	p_token parse_bool(Iterator& first, const Iterator& last)
	{
		trim_string(first,last);
		Iterator f1;
		if ((f1 = find_substr(first, last, std::string("true")))!=first) {
			first=f1;return std::make_shared<bool_const_token>(true);
		}
		if ((f1 = find_substr(first, last, std::string("True")))!=first) {
			first=f1;return std::make_shared<bool_const_token>(true);
		}
		if ((f1 = find_substr(first, last, std::string("TRUE")))!=first) {
			first=f1;return std::make_shared<bool_const_token>(true);
		}
		if ((f1 = find_substr(first, last, std::string("false")))!=first) {
			first=f1;return std::make_shared<bool_const_token>(false);
		}
		if ((f1 = find_substr(first, last, std::string("False")))!=first) {
			first=f1;return std::make_shared<bool_const_token>(false);
		}
		if ((f1 = find_substr(first, last, std::string("FALSE")))!=first) {
			first=f1;return std::make_shared<bool_const_token>(false);
		}
		return p_token();
	}
	template<class Iterator>
	p_token parse_double_or_int(Iterator& first, const Iterator& last)
	{
		int64_t number=0;
		size_t digits = 0;
		bool negative = false;
		if (first != last && *first=='-') {
			negative = true;
			++first;
		}
		while (first!=last) {
			if (*first>='0' && *first<='9') {
				number=number*10+static_cast<int64_t>(*first-'0');
				++first;
				++digits;
			} else {
				break;
			}
		}
		if (first == last || *first!='.') {
			if (digits) return std::make_shared<int_const_token>(number*(negative?-1:1));
			return p_token();
		}
		++first;
		long double dval = static_cast<long double>(number);
		long double pos = 0.1;
		while (first!=last) {
			if (*first>='0' && *first<='9') {
				dval = dval+pos*static_cast<long double>(*first-'0');
				pos=pos*0.1;
				++first;
				++digits;
			} else {
				break;
			}
		}
		if (digits) return std::make_shared<double_const_token>(dval*(negative?-1:1));
		return p_token();
	}
	template<class Iterator>
	p_token parse_string(Iterator& first, const Iterator& last)
	{
		trim_string(first,last);
		std::string val;
		if (first == last || *first != '"') return p_token();
		++first;
		while (first!=last) {
			if (*first=='"') break;
			val+=*first;
			++first;
		}
		if (*first != '"') return p_token();
		++first;
		return std::make_shared<string_const_token>(val);
	}
	template<class Iterator>
	p_token parse_dict(Iterator& first, const Iterator& last)
	{
		trim_string(first,last);
		if (first == last || *first != '{') return p_token();
		++first;
		auto dict = std::make_shared<dict_const_token>();
		while(first!=last) {
			trim_string(first,last);
			auto id = find_id(first,last);
			if (id.empty()) break;// return p_token();
			trim_string(first,last);
			if (first == last || *first!=':') return p_token();
			++first;
			auto p = parse_arg(first,last);
			if (!p) return p_token();
			dict->members.insert(std::make_pair(id,p));
			trim_string(first,last);
			if (first == last || *first != ',') break;
			++first;
		}
		if (first == last || *first != '}') return p_token();
		++first;
		return dict;
	}
	template<class Iterator>
	p_token parse_null(Iterator& first, const Iterator& last)
	{
		trim_string(first,last);
		Iterator f1;
		if ((f1 = find_substr(first, last, std::string("null")))!=first) {
			first=f1;return std::make_shared<null_const_token>();
		}
		if ((f1 = find_substr(first, last, std::string("NULL")))!=first) {
			first=f1;return std::make_shared<null_const_token>();
		}
		if ((f1 = find_substr(first, last, std::string("Null")))!=first) {
			first=f1;return std::make_shared<null_const_token>();
		}
		return p_token();
	}
	template<class Iterator>
	p_token parse_bang(Iterator& first, const Iterator& last)
	{
		trim_string(first,last);
		Iterator f1;
		if ((f1 = find_substr(first, last, std::string("bang")))!=first) {
			first=f1;return std::make_shared<bang_const_token>();
		}
		if ((f1 = find_substr(first, last, std::string("BANG")))!=first) {
			first=f1;return std::make_shared<bang_const_token>();
		}
		if ((f1 = find_substr(first, last, std::string("Bang")))!=first) {
			first=f1;return std::make_shared<bang_const_token>();
		}
		return p_token();
	}
	template<class Iterator>
	p_token parse_vector(Iterator& first, const Iterator& last)
	{
		trim_string(first,last);
		if (first == last || *first != '[') return p_token();
		++first;
		auto vec = std::make_shared<vector_const_token>();
		while(first!=last) {
			trim_string(first,last);
			auto p = parse_arg(first,last);
			if (!p) break;//return p_token();
			vec->members.push_back(p);
			trim_string(first,last);
			if (first == last || *first != ',') break;
			++first;
		}
		if (*first != ']') return p_token();
		++first;
		return vec;
	}
	template<class Iterator>
	p_token parse_const(Iterator& first, const Iterator& last)
	{
		Iterator f1 = first;
		if (auto p = parse_bool(f1,last)) {
			first = f1;
			return p;
		}
		f1 = first;
		if (auto p = parse_double_or_int(f1,last)) {
			first = f1;
			return p;
		}
		f1 = first;
		if (auto p = parse_string(f1,last)) {
			first = f1;
			return p;
		}
		f1 = first;
		if (auto p = parse_vector(f1,last)) {
			first = f1;
			return p;
		}
		f1 = first;
		if (auto p = parse_dict(f1,last)) {
			first = f1;
			return p;
		}
		f1 = first;
		if (auto p = parse_null(f1,last)) {
			first = f1;
			return p;
		}
		f1 = first;
		if (auto p = parse_bang(f1,last)) {
			first = f1;
			return p;
		}
		return p_token();
	}
	template<class Iterator>
	p_token parse_arg(Iterator& first, const Iterator& last)
	{
		trim_string(first,last);
		{
			Iterator f1 = first;
			auto p = parse_expr(f1,last);
			if (p) {
				first = f1;
				return p;
			}
		}
		{
			Iterator f1 = first;
			auto p = parse_const(f1,last);
			if (p) {
				first = f1;
				return p;
			}
		}
		return p_token();

	}
	template<class Iterator>
	p_token parse_func(Iterator& first, const Iterator& last)
	{
		std::string fname{};
		fname = find_id(first,last);
		if (fname.empty()) return p_token();
		trim_string(first,last);
		if (first == last || *first != '(') return p_token();
		++first;
		auto func = std::make_shared<func_token>(fname);
		trim_string(first,last);
		while(first != last) {
			auto p = parse_arg(first,last);
			if(!p) return p_token();
			func->args.push_back(p);
			if (*first != ',') break;
			++first;
		}
		if (*first != ')') return p_token();
		++first;

		return func;
	}
	template<class Iterator>
	p_token parse_expr(Iterator& first, const Iterator& last)
	{
		trim_string(first,last);
		Iterator f1 = first;
		if (auto p = parse_spec(f1, last)) {
			first = f1;
			return p;
		}
	
		f1 = first;
		if (auto p = parse_func(f1, last)) {
			first = f1;
			return p;
		}

		return p_token();

	}

	template<class Iterator>
	p_token parse_route(Iterator& first, const Iterator& last)
	{
		trim_string(first, last);
		Iterator f2;
		if ((f2 = find_substr(first, last, route_lex))!=first) {
			first = f2;
			trim_string(first,last);
			if (first == last || *first != '(') return p_token();
			++first;
			auto route = std::make_shared<route_token>();
			route->expr = parse_expr(first, last);
			if (!route->expr) return p_token();
			trim_string(first,last);
			if (first == last || *first != ')') return p_token();
			++first;
			trim_string(first,last);
			Iterator f3 = find_substr(first,last,std::string("->"));
			if (f3==first) return p_token();
			first = f3;
			while(first!=last) {
				auto spec = parse_spec(first,last);
				if (!spec) return p_token();
				route->output.push_back(spec);
				trim_string(first,last);
				if (first != last && *first==';') {
					++first;
					break;
				}
				if (first == last || *first!=',') return p_token();
				++first;
			}
			return route;
		}
		return p_token();
	}
}
bool is_simple_route(const p_token& ast)
{
	auto route = std::dynamic_pointer_cast<route_token>(ast);
	if (!route) return false;
	assert (route->expr);
	if (route->expr->type == token_type_t::spec) return true;
	return false;
}

std::pair<std::vector<p_token>, std::string> parse_string(const std::string& text)
{
	auto first =text.begin();
	const auto last = text.end();
	std::vector<p_token> routes;
	while (first!=last) {
		if (auto p = parser::parse_route(first, last)) {
			routes.push_back(p);
		} else break;
	}
	trim_string(first,last);
	return {routes, std::string(first,last)};
}
p_token parse_expr_string(const std::string& text)
{
	auto first =text.begin();
	const auto last = text.end();
	if (p_token p = parser::parse_const(first,last)) {
		if (first == last) return p;
	}
	first =text.begin();
	if (p_token tok = parser::parse_expr(first, last)) {
		if (first == last) return tok;
	}
	return p_token();
}

}


BasicEventParser::BasicEventParser(log::Log& log)
:BasicEventProducer(log),BasicEventConsumer(log),log_pa_(log)
{
}


namespace {

	struct value_provider {
		virtual pBasicEvent get_value(const EventRouter& router) const = 0;
		virtual ~value_provider() {}
	};
	struct null_value: public value_provider {
		null_value():value_provider(){}
		pBasicEvent get_value(const EventRouter& /*router*/) const {
			return pBasicEvent();
		}
	};
	struct bang_value: public value_provider {
		bang_value():value_provider(),value(new EventBang()){}
		pBasicEvent get_value(const EventRouter& /*router*/) const {
			return value;
		}
		pBasicEvent value;

	};
	struct const_value: public value_provider {
		const_value(const pBasicEvent& value):value_provider(),value(value) {}
		pBasicEvent value;
		pBasicEvent get_value(const EventRouter& /*router*/) const {
			return value;
		}
	};
	struct vec_value: public value_provider {
		vec_value():value_provider() {}
		std::vector<std::unique_ptr<value_provider>> inputs;
		pBasicEvent get_value(const EventRouter& router) const {
			auto vec = std::make_shared<EventVector>();
			for (const auto& in: inputs) {
				assert(in);
				vec->push_back(in->get_value(router));
			}
			return vec;
		}
	};
	struct dict_value: public value_provider {
		dict_value():value_provider() {}
		std::map<std::string, std::unique_ptr<value_provider>> inputs;
		pBasicEvent get_value(const EventRouter& router) const {
			auto dict = std::make_shared<EventDict>();
			auto& dmap = dict->get_value();
			for (const auto& in: inputs) {
				dmap[in.first] = (in.second)->get_value(router);
			}
			return dict;
		}
	};
	struct event_value: public value_provider {
		event_value(const std::string& node, const std::string& name):value_provider(),
				name(node+":"+name) {}
		std::string name;
		std::unique_ptr<value_provider> init;
		pBasicEvent get_value(const EventRouter& router) const;
	};
	struct func_call: public value_provider {
		func_call(const std::string& fname, parser::func_mode_t mode):value_provider(),
				fname(fname),mode(mode) {}
		std::string fname;
		parser::func_mode_t mode;
		std::vector<std::unique_ptr<value_provider>> inputs;

		pBasicEvent get_value(const EventRouter& router) const {
			std::vector<pBasicEvent> in;
//			std::cout << "func_call::get_value()  (" << inputs.size()<< "\n";
			for (const auto& input: inputs) {
				assert(input);
				in.push_back(input->get_value(router));
			}
			return call(fname,in);
		}
	};
	template<class token_type, class event_type>
	std::unique_ptr<const_value> process_const (const parser::p_token& token)
	{
		const auto& const_token = std::dynamic_pointer_cast<token_type>(token);
		assert(const_token);
		return std::unique_ptr<const_value>(new const_value(std::make_shared<event_type>(const_token->val)));
	}
}
	class EventRouter: public BasicEventConsumer, public BasicEventProducer
	{
	public:
		EventRouter(const parser::p_token& token, log::Log& log)
			:BasicEventConsumer(log),BasicEventProducer(log),log_er_(log)
		{
			(void)log_er_;
//			assert(token->type == parser::token_type_t::func_name);
			provider = process_tree(token);

		}
		pBasicEvent get_value(const std::string& name) const
		{
//			log_er_[log::info] << "Looking up value " << name;
			auto it = input_events.find(name);
			if (it==input_events.end()) return pBasicEvent();
//			log_er_[log::info] << "found " << name;
			return it->second;
//			throw std::runtime_error("No value");
		}
		void process() {
			process_events();
		}
		pBasicEvent get_event()
		{
			try {
//				log_er_[log::info] << "GE";
				return provider->get_value(*this);
			}
			catch (std::runtime_error& /* e */)
			{
				//log_er_[log::info] << "BOO: " << e.what();
				throw;
			}
		}
		void try_emit_event() {
//			try {
//				std::cout << "emit impl\n";
				emit_event("out",get_event());
//				std::cout << "emit done\n";
//			}
//			catch (std::runtime_error& e)
//			{
//				std::cout << "Error " << e.what() << "\n";
//				throw;
//			}
		}

	private:
		log::Log&	log_er_;
		std::unique_ptr<value_provider> process_tree(const parser::p_token& ast)
		{
			switch (ast->type) {
				case parser::token_type_t::int_const: return process_const<parser::int_const_token, EventInt>(ast);
				case parser::token_type_t::double_const: return process_const<parser::double_const_token, EventDouble>(ast);
				case parser::token_type_t::bool_const: return process_const<parser::bool_const_token, EventBool>(ast);
				case parser::token_type_t::string_const: return process_const<parser::string_const_token, EventString>(ast);
				case parser::token_type_t::bang_const: return make_unique<bang_value>();
				// TODO: Implement other constants
				case parser::token_type_t::spec: {
//					std::cout << "Processing spec\n";
					const auto& token = std::dynamic_pointer_cast<parser::spec_token>(ast);
					input_map[{token->node, token->name}]=token->node+":"+token->name;
					std::unique_ptr<event_value> spec (new event_value(token->node, token->name));
					if (token->init) {
						spec->init = process_tree(token->init);
					}
					return spec;
				}
				case parser::token_type_t::func_name: {
//					std::cout << "Processing func\n";
					const auto& token = std::dynamic_pointer_cast<parser::func_token>(ast);
					std::unique_ptr<func_call> func {new func_call(token->fname, token->mode)};
					for (const auto& arg: token->args) {
						func->inputs.push_back(process_tree(arg));
					}
					return func;
				}
				case parser::token_type_t::vector_const: {
//					std::cout << "New vector const\n";
					const auto& token = std::dynamic_pointer_cast<parser::vector_const_token>(ast);
					std::unique_ptr<vec_value> vec {new vec_value()};
					for (const auto& arg: token->members) {
						vec->inputs.push_back(process_tree(arg));
					}
					return vec;
				}
				case parser::token_type_t::dict_const: {
					const auto& token = std::dynamic_pointer_cast<parser::dict_const_token>(ast);
					std::unique_ptr<dict_value> dict {new dict_value()};
					for (const auto& arg: token->members) {
						dict->inputs[arg.first] = process_tree(arg.second);
					}
					return dict;
				}
				case parser::token_type_t::null_const: {
					std::unique_ptr<null_value> n (new null_value{});
					assert(n);
					return n;
				}
				default: break;
			}

			throw std::runtime_error("Bad tree!");
		}
		virtual bool do_process_event(const std::string& event_name, const pBasicEvent& event)
		{
			input_events[event_name] = event;
			// TODO: This should depend on 'mode' attribute...
			try_emit_event();
			return true;
		}
		std::map<std::string, pBasicEvent> input_events;
		std::unique_ptr<value_provider> provider;
	public:
		std::map<std::pair<std::string, std::string>, std::string> input_map;
	};
namespace {
	pBasicEvent event_value::get_value(const EventRouter& router) const {
		if (auto event = router.get_value(name)) return event;
		if (init) return init->get_value(router);
		throw std::runtime_error("bah");

	}
}
pBasicEvent BasicEventParser::parse_const(const std::string& text)
{
//	log_pa_[log::info] << "Parsing " << text;
	if (parser::p_token token = parser::parse_expr_string(text)) {
		EventRouter r(token, log_pa_);
		try {
			return r.get_event();
		}
		catch (std::runtime_error&) {
			return {};
		}
	}
//	log_pa_[log::info] << "No event";
	return {};
}
pBasicEvent BasicEventParser::parse_expr(log::Log& l, const std::string& text, const std::map<std::string, pBasicEvent>& inputs)
{
//	l[log::info] << "Parsing " << text;
	if (parser::p_token token = parser::parse_expr_string(text)) {
		EventRouter r(token, l);
		for (const auto& inp: inputs) {
//			l[log::info] << "Pushing event " << inp.first;
			r.receive_event("@:"+inp.first, inp.second);
		}
		r.process();
		try {
			return r.get_event();
		}
		catch (std::runtime_error&) {
			return {};
		}
	}
//	l[log::info] << "No event";
	return {};
}


bool BasicEventParser::parse_routes(const std::string& text)
{
	auto p = parser::parse_string(text);
	if (!p.second.empty()) {
		log_pa_[log::warning] << "Unparsed string: " << p.second;
	}
	const auto& routes = p.first;
	if (routes.empty()) return false;
	for (const auto& r: routes) {
		if (is_simple_route(r)) {
			auto route = std::dynamic_pointer_cast<parser::route_token>(r);
			if (!route) break; // This should never happen!
			auto spec = std::dynamic_pointer_cast<parser::spec_token>(route->expr);
			if (!spec) break; // This should never happen!
			if (pBasicEventProducer producer = find_producer(spec->node)) {
				for (const auto& output: route->output) {
					auto ospec = std::dynamic_pointer_cast<parser::spec_token>(output);
					if (!ospec) continue;
					pBasicEventConsumer consumer = find_consumer(ospec->node);
					if (!consumer) continue;
					//logp[log::debug] << "Connecting"
					producer->register_listener(spec->name, consumer, ospec->name);
				}
			} else {
				// log << "Failed to get producer";
			}
		} else {
			try {
//				std::cout << "Trying to create router\n";
				auto route = std::dynamic_pointer_cast<parser::route_token>(r);
				if (route->expr->type!=parser::token_type_t::func_name) continue;
				if (route->output.empty()) continue;
				auto func = std::dynamic_pointer_cast<parser::func_token>(route->expr);
				if (!func) continue;
//				std::cout << "Got func: " << func->fname << "\n";
				auto f = std::make_shared<EventRouter>(std::dynamic_pointer_cast<parser::func_token>(func),log_pa_);
//				std::cout << "Number of params: " << f->input_map.size() << "\n";
				for (const auto& input: f->input_map) {

					if (pBasicEventProducer producer = find_producer(input.first.first)) {
						producer->register_listener(input.first.second,f,input.second);
//						std::cout << "Connected producer " << input.first.first << " to router\n";
					}
				}
//				std::cout << "Number of outputs: " << route->output.size() << "\n";
				for (const auto& out: route->output) {
					if (out->type != parser::token_type_t::spec) continue;
//					std::cout << "Got spec\n";
					auto spec = std::dynamic_pointer_cast<parser::spec_token>(out);
					pBasicEventConsumer consumer = find_consumer(spec->node);
					if (!consumer) continue;
//					std::cout << "Got consumer\n";
					f->register_listener("out",consumer, spec->name);
//					std::cout << "Connected router to consumer " << spec->node << "\n";

				}
				// Let's try to emit event at startup. This will succeed iff the expression is constant or every variable (spec) has an initializer
//				std::cout << "Emitting startup value\n";
				if (f->input_map.empty())
					f->try_emit_event();
//				std::cout << "done\n";
				routers_.push_back(f);

			}
			catch (std::runtime_error& /* e */)
			{
//				std::cout << "Error: " << e.what() << "\n";
			}
		}
	}
	return true;
}

bool BasicEventParser::run_routers()
{
	for (auto& router: routers_) {
		router->process();
	}
	return true;
}
bool BasicEventParser::do_process_event(const std::string& /*event_name*/, const event::pBasicEvent& /*event*/)
{
	return true;
}

}
}

