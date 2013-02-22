/*!
 * @file 		Flip.cpp
 * @author 		Zdenek Travnicek
 * @date 		16.3.2012
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2012 - 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#include "Flip.h"
#include "yuri/core/Module.h"
namespace yuri {

namespace io {

REGISTER("flip",Flip)

IO_THREAD_GENERATOR(Flip)

core::pParameters Flip::configure()
{
	core::pParameters p = BasicIOThread::configure();
	(*p)["flip_x"]["flip x (around y axis)"]=true;
	(*p)["flip_y"]["flip y (around X axis)"]=false;
	p->set_max_pipes(1,1);
	//p->add_input_format(YURI_FMT_RGB);
	//p->add_output_format(YURI_FMT_RGB);
	return p;
}


Flip::Flip(log::Log &_log, core::pwThreadBase parent,core::Parameters &parameters) IO_THREAD_CONSTRUCTOR
		:core::BasicIOThread(_log,parent,1,1),flip_x(true),flip_y(false)
 {
	IO_THREAD_INIT("Flip")

}

Flip::~Flip() {

}


bool Flip::step()
{
	core::pBasicFrame  frame;
	if (!in[0] || !(frame = in[0]->pop_frame()))
		return true;

	yuri::size_t	w = frame->get_width();
	yuri::size_t	h = frame->get_height();


	core::pBasicFrame  frame_out = frame->get_copy();
	FormatInfo_t info = core::BasicPipe::get_format_info(frame->get_format());
	assert(info && info->planes==1);

	yuri::size_t Bpp = info->bpp >> 3;
	yuri::size_t line_length = w*Bpp;
	bool swap_l = false;
	if (frame->get_format()==YURI_FMT_YUV422) {
		Bpp*=2;
		swap_l=true;
	}
	if (flip_y) log[log::warning] << "Flip_y not supported!!" << std::endl;
	if (flip_x) {
		yuri::ubyte_t *base_ptr =  PLANE_RAW_DATA(frame_out,0);
//		yuri::size_t cnt = 0;
		for (yuri::size_t line =0;line<h;++line) {
			yuri::ubyte_t *in_ptr = base_ptr+line*line_length;
			yuri::ubyte_t *out_ptr = base_ptr+(line+1)*line_length-Bpp;
//			yuri::ubyte_t t;
			while (out_ptr>in_ptr) {
				for (yuri::ubyte_t b=0;b<Bpp;++b) {
					std::swap(*in_ptr++,*out_ptr++);
				}
				if (swap_l) {
					std::swap(*(in_ptr-2),*(in_ptr-4));
					std::swap(*(out_ptr-2),*(out_ptr-4));
				}
				out_ptr-=2*Bpp;
//				t = *in_ptr;
//				*in_ptr++ = *out_ptr;
//				*out_ptr--=t;
//				cnt++;
				//std::swap(*in_ptr++,*out_ptr--);
			}
		}
//		log[log::yuri::log::info] << "swapped " << cnt << " values" << std::endl;
	}

	push_raw_video_frame(0,frame_out);
	return true;
}

bool Flip::set_param(const core::Parameter &parameter)
{
	if (parameter.name== "flip_x") {
		flip_x=parameter.get<bool>();
		log[log::info] << "flip_x is: " << flip_x<<std::endl;
	} else if (parameter.name== "flip_y") {
		flip_y=parameter.get<bool>();
	} else  return BasicIOThread::set_param(parameter);
	return true;
}

}

}
