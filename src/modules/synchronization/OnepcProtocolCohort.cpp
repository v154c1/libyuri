/*!
 * @file 		OnepcProtocolCohort.cpp
 * @author 		Anastasia Kuznetsova <kuzneana@gmail.com>
 * @date 		4. 5. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "OnepcProtocolCohort.h"
#include "yuri/event/EventHelpers.h"
#include <cmath>
namespace yuri {
namespace synchronization {

core::Parameters OnepcProtocolCohort::configure()
{
    core::Parameters p = core::IOThread::configure();
    p["fps"]["Override framerate. Specify 0 to use original, or negative value to maximal speed."]=0;
    p["central_tendency"]["Sets central tendency type. Improved average, mode, none"] = "none";
    p["frame_index"]["Using default frame index."]=false;
    return p;
}

constexpr double fps_tolerance = 0.000001;

IOTHREAD_GENERATOR(OnepcProtocolCohort)


OnepcProtocolCohort::OnepcProtocolCohort(log::Log &log_, core::pwThreadBase parent,const core::Parameters &parameters):
    core::IOThread(log_,parent,1,1,std::string("onepc_protocol_cohort")), event::BasicEventConsumer(log),
    changed_(false), global_frame_no_(1), local_frame_no_(1), id_coordinator_(0), frame_delay_(0),
    tendency_(CentralTendencyType::none), fps_(0.0), use_index_frame_(true)
{
    IOTHREAD_INIT(parameters)
}

OnepcProtocolCohort::~OnepcProtocolCohort() noexcept{}


void OnepcProtocolCohort::run()
{
    int diff = 0;
    int exp_delay = 0;
    while(still_running()){
            changed_ = false;
            //wait_for_events(10_ms);
            auto f = pop_frame(0);
            log[log::verbose_debug] << "Has frame "<<&f<<" "<<f;
            if(!f) {
            	wait_for(get_latency());
            	continue;
            }
            if(fps_ < fps_tolerance ) set_fps(f);

            while(!changed_){
                wait_for_events(get_latency());
                process_events();
                if(!still_running()) return;
            }
            exp_delay = get_delay();
            global_frame_no_ += exp_delay;

            log[log::debug] << "Shows frame "<<local_frame_no_ <<", coordinator  "<<global_frame_no_ <<" delay was "<< exp_delay;

            if(use_index_frame_){
               local_frame_no_ = f->get_index();
            }

            if(global_frame_no_ > local_frame_no_){
                diff= global_frame_no_ - local_frame_no_;
                local_frame_no_ = global_frame_no_;
                while(diff > 0){
                    f = pop_frame(0);
                    if(!f) {
                    	wait_for(get_latency());
                        continue;
                    }
                    if(!still_running()) return;
                    --diff;
                }
            }
            push_frame(0, std::move(f));
            ++local_frame_no_;
        }


//    log[log::info] << "delays ";
//    auto end = delays_.end();
//    for(auto it = delays_.begin(); it!=end; ++it){
//            log[log::info] << "Value "<<it->first <<" repeats "<< it->second <<" "<<it->first;
//    }

}

bool OnepcProtocolCohort::set_fps(const core::pFrame frame){
    if(frame->get_duration().value==0) return 0.0;
    fps_ = (1_s).value/frame->get_duration().value;
    log[log::info] << "FPS is "<<fps_;
    return true;
}



bool OnepcProtocolCohort::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
    if(iequals(event_name, "delay")){
        const int64_t value = event::lex_cast_value<int64_t>(event);
        if(fps_ > fps_tolerance) frame_delay_ = value * (fps_/1000.0);
            else frame_delay_ = static_cast<double>(value);
        std::chrono::microseconds mc (static_cast<int>((std::ceil(frame_delay_)-frame_delay_)*10));
        wait_for(duration_t(mc));
        const int64_t key = static_cast<int64_t>(frame_delay_);
        auto search = delays_.find(key);
        if(search != delays_.end()){
            delays_[key] = search->second+1;
        }else{
            delays_.insert(std::make_pair(key, 1));
        }
        return true;

    } else if(iequals(event_name,  "perform")){
        auto val = event::get_value<event::EventVector>(event);
        if(val.size()<2) {
            return false;
        }
        uint64_t id_sender = event::lex_cast_value<uint64_t>(val[0]);
        if(id_coordinator_ == 0) id_coordinator_ = id_sender;
        if(id_sender != id_coordinator_) return false;
        global_frame_no_ =  event::lex_cast_value<index_t>(val[1]);
        if(global_frame_no_ >= local_frame_no_ ) changed_ = true;
        return true;
    }
    return false;
}



int OnepcProtocolCohort::get_delay(){
    if(tendency_ == CentralTendencyType::mode){
        return calculate_mode_of_sample();
    }else if(tendency_ == CentralTendencyType::impr_average){
        return calculate_impr_average_of_sample();
    }else return frame_delay_;
}

int OnepcProtocolCohort::calculate_mode_of_sample(){
    std::pair<int64_t, int64_t> res(0,0);
    auto end = delays_.end();
    for(auto it = delays_.begin(); it!=end; ++it){
        if(res.second < it->second
                || (res.second == it->second && res.first > it->first)) {
            res=std::make_pair(it->first, it->second);
        }
    }
    return res.first;
}

int OnepcProtocolCohort::calculate_impr_average_of_sample(){
    int count =0;
    int res = 0;
    auto end = delays_.end();
    for(auto it = delays_.begin(); it!=end; ++it){
        res += (it->first*it->second);
        count = count + it->second;
    }
    return delays_.size()==0 ? 0 : res/count;
}

CentralTendencyType central_tendency_type(const std::string& descr){
    if(descr == "average") return CentralTendencyType::impr_average;
    if(descr == "mode") return CentralTendencyType::mode;
    return CentralTendencyType::none;
}


bool OnepcProtocolCohort::set_param(const core::Parameter &parameter)
{
    if (assign_parameters(parameter)
    		(fps_, "fps")
			.parsed<std::string>
    			(tendency_, "central_tendency", central_tendency_type)
			)
    	return true;
    return core::IOThread::set_param(parameter);
}

}
}
