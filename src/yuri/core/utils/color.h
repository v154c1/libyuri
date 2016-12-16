/*!
 * @file 		color.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		30. 7. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SRC_YURI_CORE_UTILS_COLOR_H_
#define SRC_YURI_CORE_UTILS_COLOR_H_

#include "yuri/core/utils/string_generator.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/utils.h"
#include <cstdint>
#include <array>
#include <limits>

namespace yuri {
namespace core {


/*
 * String representation of color_t:
 * RGB: 		#RRGGBB
 * YUV: 		#@YYUUVV
 * RGB 16bit: 	##RRRRGGGGBBBB
 * YUV 16bit:	##@YYYYUUUUVVVV
 * RGBA:		#RRGGBBAA
 * YUVA: 		#@YYUUVVAA
 * RGBA 16bit: 	##RRRRGGGGBBBBAAAA
 * YUVA 16bit:	##@YYYYUUUUVVVVAAAA
 *
 */




class color_t {
public:
	color_t():data_(0xFF),flags_(flag_rgb) {}
	color_t(const color_t&) = default;
	color_t(color_t&&) = default;
	color_t& operator=(const color_t&) = default;
	color_t& operator=(color_t&&) = default;

	static color_t create_rgb(uint8_t r, uint8_t g, uint8_t b);
	static color_t create_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	static color_t create_rgb16(uint16_t r, uint16_t g, uint16_t b);
	static color_t create_rgba16(uint16_t r, uint16_t g, uint16_t b, uint16_t a);

	static color_t create_yuv(uint8_t y, uint8_t u, uint8_t v);
	static color_t create_yuva(uint8_t y, uint8_t u, uint8_t v, uint8_t a);
	static color_t create_yuv16(uint16_t y, uint16_t u, uint16_t v);
	static color_t create_yuva16(uint16_t y, uint16_t u, uint16_t v, uint16_t a);

	template<typename T, size_t dim>
	static color_t create_rgb(const std::array<T, dim>&);

	template<typename T, size_t dim>
	static color_t create_yuv(const std::array<T, dim>&);


	color_t& set_rgb(uint8_t r, uint8_t g, uint8_t b);
	color_t& set_rgb16(uint16_t r, uint16_t g, uint16_t b);
	color_t& set_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	color_t& set_rgba16(uint16_t r, uint16_t g, uint16_t b, uint16_t a);


	color_t& set_yuv(uint8_t y, uint8_t u, uint8_t v);
	color_t& set_yuv16(uint16_t y, uint16_t u, uint16_t v);
	color_t& set_yuva(uint8_t y, uint8_t u, uint8_t v, uint8_t a);
	color_t& set_yuva16(uint16_t y, uint16_t u, uint16_t v, uint16_t a);

	uint8_t r() const;
	uint8_t g() const;
	uint8_t b() const;
	uint8_t a() const;
	uint8_t y() const;
	uint8_t u() const;
	uint8_t v() const;

	uint16_t r16() const;
	uint16_t g16() const;
	uint16_t b16() const;
	uint16_t a16() const;
	uint16_t y16() const;
	uint16_t u16() const;
	uint16_t v16() const;

	std::array<uint8_t, 3> get_rgb() const;
	std::array<uint8_t, 3> get_yuv() const;
	std::array<uint8_t, 4> get_rgba() const;
	std::array<uint8_t, 4> get_yuva() const;

	std::array<uint16_t, 3> get_rgb16() const;
	std::array<uint16_t, 3> get_yuv16() const;
	std::array<uint16_t, 4> get_rgba16() const;
	std::array<uint16_t, 4> get_yuva16() const;

	void convert_to_rgb();
	void convert_to_yuv();
	void convert_to_rgba();
	void convert_to_yuva();

	void convert_to_rgb16();
	void convert_to_yuv16();
	void convert_to_rgba16();
	void convert_to_yuva16();

	std::ostream& print_value(std::ostream& os) const;

	bool operator==(const color_t& other) const;

	bool operator!=(const color_t& other) const
	{
		return !(*this == other);
	}

private:

	template<typename T, typename T2>
	static typename std::enable_if<sizeof(T) == sizeof(T2), T>::type
	set_value(T2 val) {
		return val;
	}

	template<typename T, typename T2>
	static typename std::enable_if<(sizeof(T) < sizeof(T2)), T>::type
	set_value(T2 val) {
		return static_cast<T>(val >> ((sizeof(T2) - sizeof(T))*8));
	}

	template<typename T, typename T2>
	static typename std::enable_if<(sizeof(T) > sizeof(T2)), T>::type
	set_value(T2 val) {
		return static_cast<T>(val) << ((sizeof(T) - sizeof(T2))*8);
	}

	template<typename T, size_t pos>
	T get_value() const {
		if (flags_ & flag_16bit) {
			return get_value<T, uint16_t, pos>();
		}
		return get_value<T, uint8_t, pos>();
	}

	template<typename T, typename T2, size_t pos>
	T get_value() const {
		const auto shift = sizeof(T2) * 8 * (3 - pos);
		const T2 mask = ~(T2{});
		return set_value<T, T2>(static_cast<T2>((data_ >> shift) & mask));
	}

	template<typename T, typename T2>
	T get_value(size_t pos) const {
		const auto shift = sizeof(T2) * 8 * (3 - pos);
		const T2 mask = ~(T2{});
		return set_value<T, T2>(static_cast<T2>((data_ >> shift) & mask));
	}

	template<typename T, typename T2, size_t dim>
	void set_values(std::array<T, dim>& out) const {
		for (auto i: irange(dim)) {
			out[i] = get_value<T, T2>(i);
		}
	}

	template<typename T, size_t dim>
	void set_values(std::array<T, dim>& out) const {
		if (flags_ & flag_16bit) {
			set_values<T, uint16_t, dim>(out);
		} else {
			set_values<T, uint8_t, dim>(out);
		}
	}

	template<typename T, size_t dim>
	std::array<T, dim> get_rgb_impl() const;

	template<typename T, size_t dim>
	std::array<T, dim> get_yuv_impl() const;
private:
	static const uint64_t flag_rgb = 		0x0001;
	static const uint64_t flag_yuv = 		0x0002;
	static const uint64_t flag_csp = 		0x000F;

	static const uint64_t flag_16bit = 		0x0010;
	static const uint64_t flag_alpha = 		0x0020;

	uint64_t data_;
	uint64_t flags_;
};


std::ostream& operator<<(std::ostream& os, const color_t& col);
std::istream& operator>>(std::istream& os, color_t& col);

/*!
 * Computes color difference as a distance in RGB colorspace
 * @param color1
 * @param color2
 * @return difference of the two colors
 */
double color_distance_rgb(const color_t& color1, const color_t& color2);

/*!
 * Computes color difference as a distance in YUV colorspace (probably meaningless)
 * @param color1
 * @param color2
 * @return difference of the two colors
 */
double color_distance_yuv(const color_t& color1, const color_t& color2);


template<>
inline color_t color_t::create_yuv<uint8_t, 3>(const std::array<uint8_t, 3>& col)
{
	return color_t::create_yuv(col[0], col[1], col[2]);
}

template<>
inline color_t color_t::create_yuv<uint8_t, 4>(const std::array<uint8_t, 4>& col)
{
	return color_t::create_yuva(col[0], col[1], col[2], col[3]);
}

template<>
inline color_t color_t::create_yuv<uint16_t, 3>(const std::array<uint16_t, 3>& col)
{
	return color_t::create_yuv16(col[0], col[1], col[2]);
}

template<>
inline color_t color_t::create_yuv<uint16_t, 4>(const std::array<uint16_t, 4>& col)
{
	return color_t::create_yuva16(col[0], col[1], col[2], col[3]);
}


template<>
inline color_t color_t::create_rgb<uint8_t,3>(const std::array<uint8_t, 3>& col)
{
	return color_t::create_rgb(col[0], col[1], col[2]);
}
template<>
inline color_t color_t::create_rgb<uint8_t,4>(const std::array<uint8_t, 4>& col)
{
	return color_t::create_rgba(col[0], col[1], col[2], col[3]);
}
template<>
inline color_t color_t::create_rgb<uint16_t,3>(const std::array<uint16_t, 3>& col)
{
	return color_t::create_rgb16(col[0], col[1], col[2]);
}
template<>
inline color_t color_t::create_rgb<uint16_t,4>(const std::array<uint16_t, 4>& col)
{
	return color_t::create_rgba16(col[0], col[1], col[2], col[3]);
}


}
}




#endif /* SRC_YURI_CORE_UTILS_COLOR_H_ */
