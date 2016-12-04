/*!
 * @file 		onepc_protocol_test.cpp
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		4. 5. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "tests/catch.hpp"
#include "OnepcProtocolCoordinator.h"
#include "OnepcProtocolCohort.h"
#include <iostream>

namespace yuri {
namespace synchronization {

namespace {
class F: public core::Frame
{
public:
    F(): core::Frame(0) {}
private:
    virtual core::pFrame do_get_copy() const { return std::make_shared<F>(); };
    virtual size_t	do_get_size() const noexcept { return 0; }
};


   std::stringstream ss;
   log::Log l(ss);

//   auto p1 = core::NonBlockingSingleFramePipe::generate("p1", l, core::NonBlockingSingleFramePipe::configure());
//   auto p2 = core::NonBlockingSingleFramePipe::generate("p2", l, core::NonBlockingSingleFramePipe::configure());

   auto frame = std::make_shared<F>();
   auto cohort = std::static_pointer_cast<OnepcProtocolCohort>
           (OnepcProtocolCohort::generate(l, core::pwThreadBase{}, OnepcProtocolCohort::configure()));

    std::unordered_map<int64_t, int64_t> test_map1 {};
    std::unordered_map<int64_t, int64_t> test_map2 { {4, 3}, {1, 6}, {2, 2}};
    std::unordered_map<int64_t, int64_t> test_map3 { {1, 3}, {6, 3}, {2, 3}, {3, 1}, {4, 1}};
    std::unordered_map<int64_t, int64_t> test_map4 { {0, 8}, {20, 1}, {8, 5}, {2, 6}, {3, 4}};

   void test_set_fps(const duration_t& duration, const int& res){
           frame->set_duration(duration);
           cohort->set_fps(frame);
           std::cout<<cohort->get_fps()<< "  " << frame->get_duration()<<std::endl;
           REQUIRE( (int) cohort->get_fps() == res );
   }

   void test_calculate_mode_of_sample(const std::unordered_map<int64_t, int64_t> test_data, const int& res){
           cohort->set_delays(test_data);
           REQUIRE(cohort->get_delays().size() == test_data.size());
           REQUIRE(cohort->calculate_mode_of_sample()==res);
   }

   void test_calculate_median_of_sample(const std::unordered_map<int64_t, int64_t> test_data, const int& res, const std::string& name){
           SECTION(name){
               cohort->set_delays(test_data);
               REQUIRE(cohort->get_delays().size() == test_data.size());
               REQUIRE(cohort->calculate_impr_average_of_sample()==res);
           }
   }
}


    /**
    * The test verifies a calculation and setting of fps.
    * @brief TEST_CASE
    */
   TEST_CASE("Set fps", "[module]"){
       test_set_fps(500_ms, 2);
       test_set_fps(1_s, 1);
       test_set_fps(33.3_ms, 30);
       test_set_fps(40_ms, 25);
   }

   /**
    * The test verifies a calculation of central tendency.
    * @brief TEST_CASE
    */
   TEST_CASE("Calculate mode of sample", "[module]"){
       test_calculate_mode_of_sample(test_map1, 0);
       test_calculate_mode_of_sample(test_map2, 1);
       test_calculate_mode_of_sample(test_map3, 1);
       test_calculate_mode_of_sample(test_map4, 0);
   }

   /**
    * The test verifies a calculation of central tendency.
    * @brief TEST_CASE
    */
   TEST_CASE("Calculate median of sample", "[module]"){
       test_calculate_median_of_sample(test_map1, 0, "First case");
       test_calculate_median_of_sample(test_map2, 2, "Second case");
       test_calculate_median_of_sample(test_map3, 3, "Third case");
       test_calculate_median_of_sample(test_map4, 3, "Fourth case");
   }



}}
