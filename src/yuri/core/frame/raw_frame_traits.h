 /*!
 * @file 		raw_frame_traits.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		7.2.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */


#ifndef RAW_VIDEO_TRAITS_H_
#define RAW_VIDEO_TRAITS_H_

#include "raw_frame_types.h"
#include <type_traits>

namespace yuri {
namespace core {
namespace raw_format {

template<format_t>
struct frame_traits {
// No default implementation
	static const bool is_specialized = false;
};


struct frame_traits_8bit_single {

	static const bool is_specialized = true;
	using component_type = uint8_t;
	static const int component_count = 1;
	static const int bits_per_pixel = 8;
};

struct frame_traits_16bit_single {
	static const bool is_specialized = true;
	using component_type = uint16_t;
	static const int component_count = 1;
	static const int bits_per_pixel = 16;
};

struct frame_traits_8bit_two_comp {
	static const bool is_specialized = true;
	using component_type = uint8_t;
	static const int component_count = 2;
	static const int bits_per_pixel = 16;
};

struct frame_traits_16bit_two_comp {
	static const bool is_specialized = true;
	using component_type = uint16_t;
	static const int component_count = 2;
	static const int bits_per_pixel = 32;
};

struct frame_traits_8bit_three_comp {
	static const bool is_specialized = true;
	using component_type = uint8_t;
	static const int component_count = 3;
	static const int bits_per_pixel = 24;
};

struct frame_traits_16bit_three_comp {
	static const bool is_specialized = true;
	using component_type = uint16_t;
	static const int component_count = 3;
	static const int bits_per_pixel = 48;
};

struct frame_traits_8bit_four_comp {
	static const bool is_specialized = true;
	using component_type = uint8_t;
	static const int component_count = 4;
	static const int bits_per_pixel = 32;
};

struct frame_traits_16bit_four_comp {
	static const bool is_specialized = true;
	using component_type = uint16_t;
	static const int component_count = 4;
	static const int bits_per_pixel = 64;
};

struct frame_traits_no_subsampling {
	static const bool is_subsampled = false;
	// How many pixel are needed to get a color
	static const int complete_pixel = 1;
};
template<>
struct frame_traits<y8>: public frame_traits_8bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<u8>: public frame_traits_8bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<v8>: public frame_traits_8bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<r8>: public frame_traits_8bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<g8>: public frame_traits_8bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<b8>: public frame_traits_8bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<alpha8>: public frame_traits_8bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<depth8>: public frame_traits_8bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<y16>: public frame_traits_16bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<u16>: public frame_traits_16bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<v16>: public frame_traits_16bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<r16>: public frame_traits_16bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<g16>: public frame_traits_16bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<b16>: public frame_traits_16bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<alpha16>: public frame_traits_16bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<depth16>: public frame_traits_16bit_single , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<rgb24>: public frame_traits_8bit_three_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<bgr24>: public frame_traits_8bit_three_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<gbr24>: public frame_traits_8bit_three_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<rgba32>: public frame_traits_8bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<bgra32>: public frame_traits_8bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<argb32>: public frame_traits_8bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<abgr32>: public frame_traits_8bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<rgb48>: public frame_traits_16bit_three_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<bgr48>: public frame_traits_16bit_three_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<rgba64>: public frame_traits_16bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<bgra64>: public frame_traits_16bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<argb64>: public frame_traits_16bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<abgr64>: public frame_traits_16bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<yuv444>: public frame_traits_8bit_three_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<ayuv4444>: public frame_traits_8bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<yuva4444>: public frame_traits_8bit_four_comp , public frame_traits_no_subsampling {
};

template<>
struct frame_traits<yuyv422>: frame_traits_8bit_two_comp {
	static const bool is_subsampled = true;
	static const int complete_pixel = 2;
};

template<>
struct frame_traits<yvyu422>: frame_traits_8bit_two_comp {
	static const bool is_subsampled = true;
	static const int complete_pixel = 2;
};

template<>
struct frame_traits<uyvy422>: frame_traits_8bit_two_comp {
	static const bool is_subsampled = true;
	static const int complete_pixel = 2;
};

template<>
struct frame_traits<vyuy422>: frame_traits_8bit_two_comp {
	static const bool is_subsampled = true;
	static const int complete_pixel = 2;
};

template<>
struct frame_traits<yuv411> {
	static const bool is_specialized = true;
	using component_type = uint8_t;
	//static const int component_count = 3;
	static const int bits_per_pixel = 12;
	static const bool is_subsampled = true;
	static const int complete_pixel = 4;
};
template<>
struct frame_traits<yvu411> {
	static const bool is_specialized = true;
	using component_type = uint8_t;
	//static const int component_count = 3;
	static const int bits_per_pixel = 12;
	static const bool is_subsampled = true;
	static const int complete_pixel = 4;
};



}
}
}



#endif /* RAW_VIDEO_TRAITS_H_ */
