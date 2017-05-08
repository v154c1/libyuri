/*!
 * @file 		Convert.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		30.10.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "Convert.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/compressed_frame_params.h"
#include "yuri/core/frame/raw_audio_frame_params.h"
#include "yuri/core/thread/ConvertUtils.h"
#include "yuri/core/thread/IOThreadGenerator.h"
#include "yuri/core/utils/Timer.h"
#include <unordered_map>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif

namespace std {
template<>
struct hash<std::pair<std::string, yuri::core::converter_key>> {
	size_t operator()(const std::pair<std::string, yuri::core::converter_key>& key) const{
		return std::hash<std::string>()(key.first) ^ std::hash<yuri::core::converter_key>()(key.second);
	}
};
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace yuri {
namespace core {


IOTHREAD_GENERATOR(Convert)

MODULE_REGISTRATION_BEGIN("convert")
		REGISTER_IOTHREAD("convert",Convert)
MODULE_REGISTRATION_END()

core::Parameters Convert::configure()
{
	core::Parameters p = core::IOFilter::configure();
	p.set_description("Convert");
	p["format"]["Target format"]="YUV";
	p["allow_passthrough"]["Allow passing the original frame though, when invalid output format is specified"]=false;
	p["threads"]["Number of threads to use (if supported by converter)"]=1;
	return p;
}


struct Convert::convert_pimpl_ {
	convert_pimpl_(log::Log& log_, size_t threads):log(log_),threads{threads} {}
	log::Log &log;
	size_t threads;



	// Returns already prepared covnerter thread of creates new and returns it.
	pConverterThread get_thread(const std::string& name, converter_key key)
	{
		auto it = stateless_threads.find(name);
		if (it!=stateless_threads.end()) return it->second;

		auto it2 = statefull_threads.find({name, key});
		if (it2!=statefull_threads.end()) return it2->second;

		const auto& gen = IOThreadGenerator::get_instance();
		if (!gen.is_registered(name)) return {};

		Parameters par = gen.configure(name);
		for (auto& p: par) {
			if (p.first == "threads")
				p.second = threads;
		}
		pIOThread iot = gen.generate(name, log, pwThreadBase{}, par);
//		log[log::info] << "converter " << name << " " << (iot?"OK":"failed");
		pConverterThread pct = std::dynamic_pointer_cast<ConverterThread>(iot);
//		log[log::info] << "pct" << name << " " << (pct?"OK":"failed");
		if (pct) {
			if (pct->converter_is_stateless()) {
				log[log::debug] << "Storing stateless converter " << name;
				stateless_threads[name]=pct;
			} else {
				if (!pct->initialize_converter(key.second)) {
					return {};
				}
				log[log::debug] << "Storing statefull converter " << name;
				statefull_threads[{name, key}]=pct;
			}
		}
		return pct;
	}

	pFrame convert_step(pFrame frame_in, const convert::convert_node_t& step) {
		pConverterThread pct = get_thread(step.name, {step.source_format, step.target_format});
		if (!pct) return {};
		return pct->convert_frame(frame_in, step.target_format);
	}


	std::unordered_map<std::string, pConverterThread> stateless_threads;
	std::unordered_map<std::pair<std::string, converter_key>, pConverterThread> statefull_threads;


};


Convert::Convert(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOFilter(log_,parent,std::string("convert")),allow_passthrough_(false)
{
	IOTHREAD_INIT(parameters)
	pimpl_.reset(new convert_pimpl_(log, threads_));
}

Convert::~Convert() noexcept
{
}

pFrame Convert::do_convert_frame(pFrame frame_in, format_t target_format)
{
	if (!target_format && allow_passthrough_) return frame_in;
	if (!target_format) return {};
	if (!frame_in) return {};
	Timer t;
	format_t source_format = frame_in->get_format();
	if (source_format == target_format) return frame_in;
	auto path = find_conversion(source_format, target_format);
	if (path.second == 0 || path.first.empty()) {
//		log[log::warning] << "Conversion not supported";
		return {};
	}
//	log[log::info] << "Path length: " << path.size();
	pFrame result = frame_in;
	for (const auto& step: path.first) {
//		log[log::info] << "Stepping to " << step.name;
		result = pimpl_->convert_step(result, step);
		if (!result) {
			log[log::info] << "Failed!";
			return {};
		}
	}
	if (result->get_duration() == 0_us) {
		result->set_duration(frame_in->get_duration());
		result->set_timestamp(frame_in->get_timestamp());
	}
	log[log::verbose_debug] << "Conversion of path with " << path.first.size() << " took " <<t.get_duration();
//	log[log::info] << "COnversion ok";
	return result;
}
pFrame Convert::convert_to_any(pFrame frame, const std::vector<format_t>& fmts)
{
	if (!frame) return {};
	format_t fmt = frame->get_format();
	if (find(fmts.begin(), fmts.end(), fmt) != fmts.end()) return frame;
	for (const auto& f: fmts) {
		pFrame frame_out = convert_frame(frame, f);
		if (frame_out) return frame_out;
	}
	return {};
}
pFrame Convert::convert_to_cheapest(pFrame frame, const std::vector<format_t>& fmts)
{
	if (!frame || fmts.empty()) return {};
	format_t fmt = frame->get_format();
	if (find(fmts.begin(), fmts.end(), fmt) != fmts.end()) return frame;
	std::vector<std::pair<format_t, size_t>> costs;
	for (const auto& f: fmts) {
		auto path = find_conversion(fmt, f);
		costs.emplace_back(std::make_pair(f, path.second));
	}
	std::sort(costs.begin(), costs.end(),
	   [](const std::pair<format_t, size_t>&a, const std::pair<format_t, size_t>& b)
	   {return a.second < b.second;});
	for (const auto&f: costs) {
		pFrame frame_out = convert_frame(frame, f.first);
		if (frame_out) return frame_out;
	}
	log[log::warning] << "No suitable conversion found";
	return {};
}
pFrame Convert::do_simple_single_step(pFrame frame)
{
	return convert_frame(std::move(frame), format_);
}

bool Convert::set_param(const core::Parameter& param)
{
	if (assign_parameters(param) //
			(allow_passthrough_, "allow_passthrough") //
			(threads_, "threads"))
		return true;
	if (param.get_name() == "format") {
		format_ = raw_format::parse_format(param.get<std::string>());
		if (!format_) format_ = compressed_frame::parse_format(param.get<std::string>());
		if (!format_) format_ = raw_audio_format::parse_format(param.get<std::string>());
	} else return core::IOFilter::set_param(param);
	return true;
}

} /* namespace convert */
} /* namespace yuri */
