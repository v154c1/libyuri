/*!
 * @file 		twopc_protocol_test.cpp
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		20. 4. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "tests/catch.hpp"
#include "TwopcProtocol.h"

namespace yuri{
namespace synchronization {
namespace {
	const float test_fps1 = 1;
    const float test_fps2 = 30;
    const float test_zero_fps = 0;
    const float test_negative_fps = -1;

    std::stringstream ss;
    log::Log l(ss);
    std::unordered_map<int, int> test_map;




    void verify_calculate_timeout(const float& fps,const duration_t& curr_timeout,
    								const duration_t& val, const std::string& name=""){
        SECTION(name) {
        auto timeout = yuri::synchronization::set_timeout(fps, curr_timeout, l);
        REQUIRE( timeout.value==val.value);
        }
    }

//    void test_cohorts_states(std::unordered_map<int, int>& map,int exp_val, int res,
//    							int max_diff=7, const std::string& name=""){
//    	SECTION(name){
//    		int map_size=yuri::twopc_protocol::control_cohorts_state(map, exp_val, max_diff);
//            REQUIRE( res==map_size );
//    	}
//    }

    void test_calculate_percentage(const int& val,const int& pct, const int& res, const std::string& name=""){
        SECTION(name){
            int percentage = yuri::synchronization::calculate_percentage(val, pct);
            REQUIRE( percentage==res );
        }
    }

}


TEST_CASE( "Verify calculate timeout", "[module]" ) {
    verify_calculate_timeout(test_fps1, 80_s, 900_ms);
    verify_calculate_timeout(test_fps1, 80_ms, 80_ms, "current timeout is less than max. timeout");
    verify_calculate_timeout(test_fps2, 80_s, 30_ms);
    verify_calculate_timeout(test_zero_fps, 80_ms, 80_ms, "timeout doesn't change when fps is nil");
    verify_calculate_timeout(test_negative_fps, 80_ms, 80_ms, "timeout doesn't change when fps is negative number");
}

//TEST_CASE( "Test cohorts_states method", "[module]" ) {

//    test_cohorts_states(test_map, 12, 1, 1);
//}

TEST_CASE( "Calculate percentage value", "[module]" ) {
    test_calculate_percentage(1, 100, 1);
    test_calculate_percentage(2, 50, 1);
    test_calculate_percentage(0, 20, 0);
    test_calculate_percentage(0, 100, 0);
    test_calculate_percentage(5, 25, 1);
    test_calculate_percentage(5, 50, 3);
}


}}
