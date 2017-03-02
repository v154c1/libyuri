/*!
 * @file 		functions.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		13.07.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_
#include "BasicEvent.h"
namespace yuri {
namespace event {
namespace functions {

pBasicEvent 					str(const std::vector<pBasicEvent>& events);
pBasicEvent 					todouble(const std::vector<pBasicEvent>& events);
pBasicEvent 					todouble_range(const std::vector<pBasicEvent>& events);
pBasicEvent 					toint(const std::vector<pBasicEvent>& events);
pBasicEvent 					toint_range(const std::vector<pBasicEvent>& events);
pBasicEvent 					tobool(const std::vector<pBasicEvent>& events);
pBasicEvent 					pass(const std::vector<pBasicEvent>& events);
pBasicEvent 					select(const std::vector<pBasicEvent>& events);


pBasicEvent 					add(const std::vector<pBasicEvent>& events);
pBasicEvent 					sub(const std::vector<pBasicEvent>& events);
pBasicEvent 					mul(const std::vector<pBasicEvent>& events);
pBasicEvent 					div(const std::vector<pBasicEvent>& events);
pBasicEvent 					mod(const std::vector<pBasicEvent>& events);
pBasicEvent 					fmod(const std::vector<pBasicEvent>& events);

pBasicEvent 					muls(const std::vector<pBasicEvent>& events);

pBasicEvent 					eq(const std::vector<pBasicEvent>& events);
pBasicEvent 					gt(const std::vector<pBasicEvent>& events);
pBasicEvent 					ge(const std::vector<pBasicEvent>& events);
pBasicEvent 					lt(const std::vector<pBasicEvent>& events);
pBasicEvent 					le(const std::vector<pBasicEvent>& events);
pBasicEvent 					abs(const std::vector<pBasicEvent>& events);

pBasicEvent 					log_and(const std::vector<pBasicEvent>& events);
pBasicEvent 					log_or(const std::vector<pBasicEvent>& events);
pBasicEvent 					log_not(const std::vector<pBasicEvent>& events);

pBasicEvent 					bit_and(const std::vector<pBasicEvent>& events);
pBasicEvent 					bit_or(const std::vector<pBasicEvent>& events);
pBasicEvent 					bit_xor(const std::vector<pBasicEvent>& events);

pBasicEvent 					min(const std::vector<pBasicEvent>& events);
pBasicEvent 					max(const std::vector<pBasicEvent>& events);

pBasicEvent 					exp(const std::vector<pBasicEvent>& events);
pBasicEvent 					pow(const std::vector<pBasicEvent>& events);
pBasicEvent 					ln(const std::vector<pBasicEvent>& events);

pBasicEvent 					geometry(const std::vector<pBasicEvent>& events);
pBasicEvent 					get_width(const std::vector<pBasicEvent>& events);
pBasicEvent 					get_height(const std::vector<pBasicEvent>& events);

pBasicEvent 					microseconds(const std::vector<pBasicEvent>& events);
pBasicEvent 					milliseconds(const std::vector<pBasicEvent>& events);
pBasicEvent 					seconds(const std::vector<pBasicEvent>& events);

pBasicEvent 					generate(const std::vector<pBasicEvent>& events);

pBasicEvent 					append(const std::vector<pBasicEvent>& events);
pBasicEvent 					slice(const std::vector<pBasicEvent>& events);

}



}
}


#endif /* FUNCTIONS_H_ */
