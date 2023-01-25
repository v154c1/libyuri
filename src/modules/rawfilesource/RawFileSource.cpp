/*!
 * @file 		RawFileSource.cpp
 * @author 		Zdenek Travnicek
 * @date 		31.3.2011
 * @date		16.3.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2011 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "RawFileSource.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/compressed_frame_params.h"
#include <fstream>
#include <boost/regex.hpp>
#include <iomanip>
namespace yuri {

namespace rawfilesource {


MODULE_REGISTRATION_BEGIN("raw_filesource")
	REGISTER_IOTHREAD("raw_filesource",RawFileSource)
MODULE_REGISTRATION_END()

IOTHREAD_GENERATOR(RawFileSource)


core::Parameters RawFileSource::configure()
{
	core::Parameters p = base_type::configure();
	p["path"]["Path to the file"]=std::string();
	p["keep_alive"]["Stay idle after pushing the file (setting to false will cause the object to quit afterward)"]=true;
	p["format"]["Force output format"]="none";
	p["width"]["Force output width to"]=0;
	p["height"]["Force output height to"]=0;
	p["chunk"]["Chunk size (0 to output whole file at once)"]=0;
	p["fps"]["Framerate for chunk output"]=25;
	p["loop"]["Start again from beginning of the file after reaching end"]=true;
	p["offset"]["skip offset bytes from beginning"]=0;
	p["block"]["Threat output pipes as blocking. Specify as max number of frames in output pipe."]=0;
	return p;
}


RawFileSource::RawFileSource(log::Log &_log, core::pwThreadBase parent,const core::Parameters &parameters):
			core::IOThread(_log,parent,1,1,"RawFileSource"), position(0),
			chunk_size(0), width(0), height(0),output_format(0),
			fps(25.0),keep_alive(true),loop(true),
			failed_read(false),sequence(false),block(0),loop_number(0),sequence_pos(0),
			frame_type_(frame_type_t::raw_video)
{
	IOTHREAD_INIT(parameters)
	set_latency(1_ms);
}

RawFileSource::~RawFileSource() noexcept {
}

void RawFileSource::run()
{
//	IOTHREAD_PRE_RUN
	while (still_running()) {
		ThreadBase::sleep(get_latency());
		if (!frame) if (!read_chunk()) break;
		if (failed_read) break;
		if (!frame) continue;
//		if (block && out_[0] && out[0]->get_count() >= block) continue;

		duration_t delta;
		if (fps!=0.0)
			delta = 1_s/fps;
		else delta = 0_s;

		if ((timestamp_t{} - last_send) < delta) continue;
		last_send+=delta;
		push_frame(0,frame);
		if (chunk_size) frame.reset();
		else if (sequence && !chunk_size) frame.reset();
		if (!loop && loop_number) break;
	}
	if (keep_alive) while (still_running()) {
		ThreadBase::sleep(get_latency());
	}
	request_end();
//	IO_THREAD_POST_RUN
}

bool RawFileSource::read_chunk()
{
	try {
		frame.reset();
		bool first_read = false;
		if (!file.is_open()) {
			std::string filepath;
			if (!sequence) {
				filepath = path;
			} else {
				filepath = next_file();
			}
			file.open(filepath.c_str(),std::ios::in|std::ios::binary);
			if (file.fail()) {
				log[log::warning] << "Failed to open " << filepath;
				if (sequence_pos) {
					log[log::info] << "Resetting sequence to the beginning";
					sequence_pos=0;
					loop_number++;
				}
				return true;
			}
			file.seekg(position,std::ios_base::beg);
			first_read = true;
		}
		yuri::size_t length = chunk_size;
		std::vector<yuri::size_t> planes= {length};
//		if (frame_type_ == frame_type_t::compressed_viceo) {
//
//
//		} else

		if (frame_type_ == frame_type_t::raw_video && width && height) {
			const auto& fi = core::raw_format::get_format_info(output_format);
			const auto bd = fi.planes[0].bit_depth;
			length = width*bd.first/bd.second/8*height;
			if (fi.planes.size() > 1) {
				planes.clear();
				for (yuri::size_t i=0;i<fi.planes.size();++i)
				{
					const auto& p = fi.planes[i];
					planes.push_back(length / p.sub_x / p.sub_y);
				}
			} else {
				planes = {length};
			}
		} else if (!chunk_size) {
			file.seekg(0,std::ios_base::end);
			length = static_cast<yuri::size_t>(file.tellg()) - position;
			file.seekg(position,std::ios_base::beg);
			planes={length};
		}

		if (frame_type_ == frame_type_t::raw_video) {
			auto rframe = core::RawVideoFrame::create_empty(output_format, {width, height});
			frame = rframe;
			for (yuri::size_t i=0;i<planes.size();++i) {
				const yuri::size_t plane_length = planes[i];
				std::istreambuf_iterator<char> it(file);

				file.read(reinterpret_cast<char*>(PLANE_RAW_DATA(rframe,i)),std::min(plane_length, PLANE_DATA(rframe,i).size()));
				if (static_cast<yuri::size_t>(file.gcount()) != plane_length ) {
					if (first_read) {
						if (!sequence || sequence_pos == 0) {
							failed_read=true;
							log[log::warning]<< "Wrong length of the file (read " << file.gcount() << ", expected " << plane_length << ")";
						} else {
							sequence_pos = 0;
						}
					}
					file.close();
					frame.reset();++loop_number;
					return !failed_read;
				}
			}

		} else if (frame_type_ == frame_type_t::compressed_viceo) {
			auto cframe = core::CompressedVideoFrame::create_empty(output_format, resolution_t{width, height}, length);
			frame = cframe;
			file.read(reinterpret_cast<char*>(cframe->get_data().data()), length);
			if (static_cast<yuri::size_t>(file.gcount()) != length ) {
				if (first_read) {
					if (!sequence || sequence_pos == 0) {
						failed_read=true;
						log[log::warning]<< "Wrong length of the file (read " << file.gcount() << ", expected " << length << ")";
					} else {
						sequence_pos = 0;
					}
				}
				file.close();
				frame.reset();++loop_number;
				return !failed_read;
			}

		}
		frame->set_duration(1_s/fps);
		if (file.eof()) {
			log[log::info] << "EOF";
			file.close();
			++loop_number;
		} else if (sequence && !chunk_size) {
			file.close();
		}
		//frame.reset(new BasicFrame(1));
		//(*frame)[0].set(data,length);
//		frame->set_parameters(output_format, width, height);
	}
	catch (std::exception &e) {
		frame.reset();
		if (sequence && sequence_pos) {
			sequence_pos = 0;
			return true;
		}
		log[log::error] << "Failed to process file " << path
				<< " (" << e.what() << ")";
		failed_read=true;
		return false;
	}
	return true;
}
bool RawFileSource::set_param(const core::Parameter &parameter)
{
	if (parameter.get_name() == "chunk") {
		chunk_size=parameter.get<yuri::size_t>();
	} else if (parameter.get_name() == "fps") {
		fps=parameter.get<double>();
	} else if (parameter.get_name() == "width") {
		width=parameter.get<yuri::size_t>();
	} else if (parameter.get_name() == "height") {
		height=parameter.get<yuri::size_t>();
	} else if (parameter.get_name() == "format") {
		auto fmt_string = parameter.get<std::string>();

		if ((output_format = core::raw_format::parse_format(fmt_string))) {
			frame_type_ = frame_type_t::raw_video;
		} else if ((output_format = core::compressed_frame::parse_format(fmt_string)))  {
			frame_type_ = frame_type_t::compressed_viceo;
		} else {
			frame_type_ = frame_type_t::unknown;
		}
		log[log::info] << "output format " << output_format;
	} else if (parameter.get_name() == "path") {
		path=parameter.get<std::string>();
		if (path.find("%")!=std::string::npos) {
			sequence = true;
		}
	} else if (parameter.get_name() == "keep_alive") {
		keep_alive=parameter.get<bool>();
	} else if (parameter.get_name() == "offset") {
		position=parameter.get<yuri::size_t>();
	} else if (parameter.get_name() == "loop") {
		loop=parameter.get<bool>();
	} else if (parameter.get_name() == "block") {
		block=parameter.get<size_t>();
	} else return base_type::set_param(parameter);
	return true;
}

std::string RawFileSource::next_file()
{
	boost::regex reg("^(.*)%([0-9]+)d(.*)$");
	boost::smatch match;
	if (!boost::regex_match(path, match, reg)) {
		log[log::error] << "sequence specification not found in " << path;
		return path;
	}
//	log[log::info] << "Sequence specs found! '"<<match[1]<<"' XXX '"<<match[2]<<"' XXX '"<<match[3]<<"'";
	size_t seq_width =0;
	try {
		seq_width = lexical_cast<size_t>(match[2]);
	}
	catch (bad_lexical_cast& ) {
		return path;
	}
	std::stringstream spath;
	spath << match[1] << std::setfill('0') << std::setw(seq_width) << sequence_pos++ << match[3];
//	log[log::info] << "Returning path " << spath.str();
	return spath.str();





}
}
}
