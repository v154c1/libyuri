/*!
 * @file 		color.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		4. 8. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "color.h"
#include <iostream>
#include <cmath>
namespace yuri {
namespace core {

namespace {
template<typename T>
uint64_t set_64b_from_components(T v0, T v1, T v2, T v3 = ~T())
{
	constexpr auto shift = sizeof(T)*8;
	return 	(static_cast<uint64_t>(v0) << (shift * 3 )) |
			(static_cast<uint64_t>(v1) << (shift * 2 )) |
			(static_cast<uint64_t>(v2) << (shift * 1 )) |
			(static_cast<uint64_t>(v3));
}

template<typename T, size_t count, size_t count_max = 4>
void print_color_values(std::ostream& os, uint64_t value)
{
	const T mask = ~(T{});
	for (auto i: irange(0, count)) {
		utils::print_formated_value(os, (value >> (8 * sizeof(T) * (count_max - i - 1))) & mask, sizeof(T) * 2, true);
	}
}

constexpr double Wr_709 	= 0.2126;
constexpr double Wb_709 	= 0.0722;
constexpr double Wg_709		= 1.0 - Wr_709 - Wb_709;
constexpr double Kr_709		= 0.5 / (1.0 - Wr_709);
constexpr double Kb_709		= 0.5 / (1.0 - Wb_709);
constexpr double WbKbWg_709	= Wb_709 / Kb_709 / Wg_709;
constexpr double WrKrWg_709	= Wr_709 / Kr_709 / Wg_709;

template<typename T, size_t dim>
std::array<T, dim> convert_to_yuv_impl(const std::array<T, dim>& rgb)
{
	std::array<T, dim> yuv;
	const std::array<double,3> rgbf = {{
			static_cast<double>(rgb[0])/std::numeric_limits<T>::max(),
			static_cast<double>(rgb[1])/std::numeric_limits<T>::max(),
			static_cast<double>(rgb[2])/std::numeric_limits<T>::max()}};

	const auto y = 	Wr_709 * rgbf[0] +
					Wg_709 * rgbf[1] +
					Wb_709 * rgbf[2];
	yuv[0] = 	static_cast<T>(std::numeric_limits<T>::max()*(
					clip_value(y, 0.0, 1.0)));

	yuv[1] = static_cast<T>(std::numeric_limits<T>::max()*(
			0.5 + clip_value((rgbf[2] - y) * Kb_709, -0.5, 0.5)));

	yuv[2] = static_cast<T>(std::numeric_limits<T>::max()*(
			0.5 + clip_value((rgbf[0] - y) * Kr_709, -0.5, 0.5)));
	if (dim > 3) {
		yuv[3] = rgb[3];
	}
	return yuv;
}

template<typename T, size_t dim>
std::array<T, dim> convert_to_rgb_impl(const std::array<T, dim>& yuv)
{
	std::array<T, dim> rgb;
	const std::array<double,3> yuvf = {{
				static_cast<double>(yuv[0])/std::numeric_limits<T>::max(),
				static_cast<double>(yuv[1])/std::numeric_limits<T>::max() - 0.5,
				static_cast<double>(yuv[2])/std::numeric_limits<T>::max() - 0.5}};
	rgb[0] = static_cast<T>(std::numeric_limits<T>::max()*
				clip_value(yuvf[0] + (yuvf[2] / Kr_709), 0.0, 1.0));
	rgb[1] = static_cast<T>(std::numeric_limits<T>::max()*
			clip_value(yuvf[0] - yuvf[2]*WrKrWg_709 - yuvf[1] * WbKbWg_709, 0.0, 1.0));
	rgb[2] = static_cast<T>(std::numeric_limits<T>::max()*
			clip_value(yuvf[0] + yuvf[1] / Kb_709, 0.0, 1.0));
	if (dim > 3) {
		rgb[3] = yuv[3];
	}
	return rgb;
}

}


color_t color_t::create_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	color_t col;
	return col.set_rgb(r, g, b);
}
color_t color_t::create_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	color_t col;
	return col.set_rgba(r, g, b, a);
}
color_t color_t::create_rgb16(uint16_t r, uint16_t g, uint16_t b)
{
	color_t col;
	return col.set_rgb16(r, g, b);
}
color_t color_t::create_rgba16(uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
	color_t col;
	return col.set_rgba16(r, g, b, a);
}

color_t color_t::create_yuv(uint8_t y, uint8_t u, uint8_t v)
{
	color_t col;
	return col.set_yuv(y, u, v);
}
color_t color_t::create_yuva(uint8_t y, uint8_t u, uint8_t v, uint8_t a)
{
	color_t col;
	return col.set_yuva(y, u, v, a);
}
color_t color_t::create_yuv16(uint16_t y, uint16_t u, uint16_t v)
{
	color_t col;
	return col.set_yuv16(y, u, v);
}
color_t color_t::create_yuva16(uint16_t y, uint16_t u, uint16_t v, uint16_t a)
{
	color_t col;
	return col.set_yuva16(y, u, v, a);
}



/* ************************************************************************** *
 *     Setters                                                                *
 * ************************************************************************** */
color_t& color_t::set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	flags_ = flag_rgb;
	data_ = set_64b_from_components(r, g, b);
	return *this;
}

color_t& color_t::set_rgb16(uint16_t r, uint16_t g, uint16_t b)
{
	flags_ = flag_rgb | flag_16bit;
	data_ = set_64b_from_components(r, g, b);
	return *this;
}

color_t& color_t::set_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	flags_ = flag_rgb | flag_alpha;
	data_ = set_64b_from_components(r, g, b, a);
	return *this;
}

color_t& color_t::set_rgba16(uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
	flags_ = flag_rgb | flag_16bit | flag_alpha;
	data_ = set_64b_from_components(r, g, b, a);
	return *this;
}

color_t& color_t::set_yuv(uint8_t y, uint8_t u, uint8_t v)
{
	flags_ = flag_yuv;
	data_ = set_64b_from_components(y, u ,v);
	return *this;
}

color_t& color_t::set_yuva(uint8_t y, uint8_t u, uint8_t v, uint8_t a)
{
	flags_ = flag_yuv | flag_alpha;
	data_ = set_64b_from_components(y, u ,v, a);
	return *this;
}

color_t& color_t::set_yuv16(uint16_t y, uint16_t u, uint16_t v)
{
	flags_ = flag_yuv | flag_16bit;
	data_ = set_64b_from_components(y, u ,v);
	return *this;
}

color_t& color_t::set_yuva16(uint16_t y, uint16_t u, uint16_t v, uint16_t a)
{
	flags_ = flag_yuv | flag_16bit | flag_alpha;
	data_ = set_64b_from_components(y, u ,v, a);
	return *this;
}
/* ************************************************************************** *
 *     Component getters                                                      *
 * ************************************************************************** */

uint8_t color_t::r() const
{
	auto ret = get_rgb();
	return ret[0];
}
uint8_t color_t::g() const
{
	auto ret = get_rgb();
	return ret[1];
}

uint8_t color_t::b() const
{
	auto ret = get_rgb();
	return ret[2];
}

uint8_t color_t::a() const
{
	if ((flags_&flag_alpha)) {
		return get_value<uint8_t, 3>();
	}
	return 0xFF;
}

uint8_t color_t::y() const
{
	auto ret = get_yuv();
	return ret[0];
}

uint8_t color_t::u() const
{
	auto ret = get_yuv();
	return ret[1];
}
uint8_t color_t::v() const
{
	auto ret = get_yuv();
	return ret[2];
}


uint16_t color_t::r16() const
{
	auto ret = get_rgb16();
	return ret[0];
}
uint16_t color_t::g16() const
{
	auto ret = get_rgb16();
	return ret[1];
}

uint16_t color_t::b16() const
{
	auto ret = get_rgb16();
	return ret[2];
}

uint16_t color_t::a16() const
{
	if ((flags_&flag_alpha)) {
		return get_value<uint16_t, 3>();
	}
	return 0xFFFF;
}

uint16_t color_t::y16() const
{
	auto ret = get_yuv16();
	return ret[0];
}

uint16_t color_t::u16() const
{
	auto ret = get_yuv16();
	return ret[1];
}
uint16_t color_t::v16() const
{
	auto ret = get_yuv16();
	return ret[2];
}


/* ************************************************************************** *
 *     Array getters                                                          *
 * ************************************************************************** */


template<typename T, size_t dim>
std::array<T, dim> color_t::get_rgb_impl() const
{
	std::array<T, dim> ret;
	if ((flags_ & flag_rgb)) {
		set_values(ret);
	} else if (flags_ & flag_yuv) {
		ret = convert_to_rgb_impl(get_yuv_impl<T, dim>());
	} else {
		throw std::range_error("Unsupported conversion");
	}
	return ret;
}

template<typename T, size_t dim>
std::array<T, dim> color_t::get_yuv_impl() const
{
	std::array<T, dim> ret;
	if ((flags_ & flag_yuv)) {
		set_values(ret);
	} else if (flags_ & flag_rgb) {
		ret = convert_to_yuv_impl(get_rgb_impl<T, dim>());
	} else {
		throw std::range_error("Unsupported conversion");
	}
	return ret;
}


std::array<uint8_t, 3> color_t::get_rgb() const
{
	return get_rgb_impl<uint8_t, 3>();
}

std::array<uint8_t, 3> color_t::get_yuv() const
{
	return get_yuv_impl<uint8_t, 3>();
}

std::array<uint8_t, 4> color_t::get_rgba() const
{
	return get_rgb_impl<uint8_t, 4>();
}

std::array<uint8_t, 4> color_t::get_yuva() const
{
	return get_yuv_impl<uint8_t, 4>();
}

std::array<uint16_t, 3> color_t::get_rgb16() const
{
	return get_rgb_impl<uint16_t, 3>();
}

std::array<uint16_t, 3> color_t::get_yuv16() const
{
	return get_yuv_impl<uint16_t, 3>();
}

std::array<uint16_t, 4> color_t::get_rgba16() const
{
	return get_rgb_impl<uint16_t, 4>();
}

std::array<uint16_t, 4> color_t::get_yuva16() const
{
	return get_yuv_impl<uint16_t, 4>();
}


/* ************************************************************************** *
 *     Conversions                                                            *
 * ************************************************************************** */

void color_t::convert_to_rgb() {
	if (flags_ == (flag_rgb)) {
		return;
	}
	const auto data = get_rgb();
	set_rgb(data[0], data[1], data[2]);
}
void color_t::convert_to_yuv()
{
	if (flags_ == (flag_yuv)) {
		return;
	}
	const auto data = get_yuv();
	set_yuv(data[0], data[1], data[2]);
}
void color_t::convert_to_rgba() {
	if (flags_ == (flag_rgb| flag_alpha)) {
		return;
	}
	const auto data = get_rgba();
	set_rgba(data[0], data[1], data[2], data[3]);
}
void color_t::convert_to_yuva()
{
	if (flags_ == (flag_yuv|flag_alpha)) {
		return;
	}
	const auto data = get_yuva();
	set_yuva(data[0], data[1], data[2], data[3]);
}

void color_t::convert_to_rgb16() {
	if (flags_ == (flag_rgb|flag_16bit)) {
		return;
	}
	const auto data = get_rgb16();
	set_rgb16(data[0], data[1], data[2]);
}
void color_t::convert_to_yuv16()
{
	if (flags_ == (flag_yuv|flag_16bit)) {
		return;
	}
	const auto data = get_yuv16();
	set_yuv16(data[0], data[1], data[2]);
}
void color_t::convert_to_rgba16() {
	if (flags_ == (flag_rgb| flag_alpha|flag_16bit)) {
		return;
	}
	const auto data = get_rgba16();
	set_rgba16(data[0], data[1], data[2], data[3]);
}
void color_t::convert_to_yuva16()
{
	if (flags_ == (flag_yuv|flag_alpha|flag_16bit)) {
		return;
	}
	const auto data = get_yuva16();
	set_yuva16(data[0], data[1], data[2], data[3]);
}


/* ************************************************************************** *
 *     Misc                                                                   *
 * ************************************************************************** */

std::ostream& color_t::print_value(std::ostream& os) const
{
	os <<std::hex;
	os << "#";
	switch(flags_ & flag_csp) {
		case flag_rgb:
			if (flags_ & flag_16bit) {
				os << "#";
				if (flags_ & flag_alpha) {
					print_color_values<uint16_t, 4>(os, data_);
				} else {
					print_color_values<uint16_t, 3>(os, data_);
				}
			} else {
				if (flags_ & flag_alpha) {
					print_color_values<uint8_t, 4>(os, data_);
				} else {
					print_color_values<uint8_t, 3>(os, data_);
				}
			}
			break;
		case flag_yuv:
			if (flags_ & flag_16bit) {
				os << "#@";
				if (flags_ & flag_alpha) {
					print_color_values<uint16_t, 4>(os, data_);
				} else {
					print_color_values<uint16_t, 3>(os, data_);
				}
			} else {
				os << "@";
				if (flags_ & flag_alpha) {
					print_color_values<uint8_t, 4>(os, data_);
				} else {
					print_color_values<uint8_t, 3>(os, data_);
				}
			}
			break;
	}
	os <<std::dec;
	return os;
}


bool color_t::operator==(const color_t& other) const {
	if (other.flags_ == flags_) {
		return other.data_ == data_;
	}
	if (flags_ & flag_16bit) {
		if (flags_ & flag_alpha) {
			if (flags_ & flag_rgb) {
				return get_rgba16() ==  other.get_rgba16();
			} else {
				return get_yuva16() ==  other.get_yuva16();
			}
		} else {
			if (flags_ & flag_rgb) {
				return get_rgb16() ==  other.get_rgb16();
			} else {
				return get_yuv16() ==  other.get_yuv16();
			}
		}
	} else {
		if (flags_ & flag_alpha) {
			if (flags_ & flag_rgb) {
				return get_rgba() ==  other.get_rgba();
			} else {
				return get_yuva() ==  other.get_yuva();
			}
		} else {
			if (flags_ & flag_rgb) {
				return get_rgb() ==  other.get_rgb();
			} else {
				return get_yuv() ==  other.get_yuv();
			}
		}
	}
}


std::ostream& operator<<(std::ostream& os, const color_t& col)
{
	return col.print_value(os);
}

namespace {
template<typename T>
T get_val_from_4bits(uint8_t*  start) {
	T ret = T{0};
	const auto steps = sizeof(T) * 2;
	for (auto i: irange(steps)) {
		(void)i;
		ret = (ret << 4) | (*start++ & 0x0F);
	}
	return ret;
}
}

std::istream& operator>>(std::istream& is, color_t& col_out)
{
	color_t col;
	char c;
	enum class state_t {
		start, data
	};
	enum class csp_t {
		rgb, rgb16, yuv, yuv16
	};

	state_t state = state_t::start;
	csp_t csp = csp_t::rgb;

	is >> c;
	if (c != '#') {
		is.setstate(std::ios::failbit);
		return is;
	}
//	std::cerr << __LINE__ << ": Prefix OK" << std::endl;
	std::vector<uint8_t> data;
	data.reserve(16);
	bool finished = false;
	while(!finished && is >> c) {

		if (state == state_t::start) {
			if (csp == csp_t::rgb && c == '#') {
				csp = csp_t::rgb16;
				continue;
			}
			if (csp == csp_t::rgb && c == '@') {
				csp = csp_t::yuv;
				continue;
			}
			if (csp == csp_t::rgb16 && c == '@') {
				csp = csp_t::yuv16;
				continue;
			}
		}
//		std::cerr << __LINE__ << ": Parsing data" << std::endl;
		state = state_t::data;
		auto cl = std::tolower(c);
		if (c >= '0' && c <= '9') {
			data.push_back(static_cast<uint8_t>(c - '0'));
		} else if (cl >= 'a' && cl <= 'f') {
			data.push_back(static_cast<uint8_t>(c - 'a') + 10);
		} else {
//			std::cerr << __LINE__ << ": BAD CHARACTER" << std::endl;
			break;
		}

		switch(csp) {
			case csp_t::rgb:
			case csp_t::yuv:
				if (data.size() >= 8) finished = true;
				break;
			case csp_t::rgb16:
			case csp_t::yuv16:
				if (data.size() >= 16) finished = true;
				break;
			default:
				break;
		}
//		std::cerr << __LINE__ << ": End of LOOP, finished: " << finished << std::endl;
//		std::cerr << __LINE__ << ": data.size(): " << data.size() << std::endl;
	}

	is.clear();

	switch (csp) {
	case csp_t::rgb:
		if (data.size() >= 8) {
			col.set_rgba(	get_val_from_4bits<uint8_t>(&data[0]),
							get_val_from_4bits<uint8_t>(&data[2]),
							get_val_from_4bits<uint8_t>(&data[4]),
							get_val_from_4bits<uint8_t>(&data[6]));
		} else if (data.size() >= 6) {
			col.set_rgb(	get_val_from_4bits<uint8_t>(&data[0]),
							get_val_from_4bits<uint8_t>(&data[2]),
							get_val_from_4bits<uint8_t>(&data[4]));

		} else {
			is.setstate(std::ios::failbit);
		} break;
	case csp_t::rgb16:
		if (data.size() >= 16) {
			col.set_rgba16(	get_val_from_4bits<uint16_t>(&data[0]),
							get_val_from_4bits<uint16_t>(&data[4]),
							get_val_from_4bits<uint16_t>(&data[8]),
							get_val_from_4bits<uint16_t>(&data[12]));
		} else if (data.size() >= 12) {
			col.set_rgb16(	get_val_from_4bits<uint16_t>(&data[0]),
							get_val_from_4bits<uint16_t>(&data[4]),
							get_val_from_4bits<uint16_t>(&data[8]));

		} else {
			is.setstate(std::ios::failbit);
		} break;
	case csp_t::yuv:
		if (data.size() >= 8) {
			col.set_yuva(	get_val_from_4bits<uint8_t>(&data[0]),
							get_val_from_4bits<uint8_t>(&data[2]),
							get_val_from_4bits<uint8_t>(&data[4]),
							get_val_from_4bits<uint8_t>(&data[6]));
		} else if (data.size() >= 6) {
			col.set_yuv(	get_val_from_4bits<uint8_t>(&data[0]),
							get_val_from_4bits<uint8_t>(&data[2]),
							get_val_from_4bits<uint8_t>(&data[4]));

		} else {
			is.setstate(std::ios::failbit);
		} break;
	case csp_t::yuv16:
		if (data.size() >= 16) {
			col.set_yuva16(	get_val_from_4bits<uint16_t>(&data[0]),
							get_val_from_4bits<uint16_t>(&data[4]),
							get_val_from_4bits<uint16_t>(&data[8]),
							get_val_from_4bits<uint16_t>(&data[12]));
		} else if (data.size() >= 12) {
			col.set_yuv16(	get_val_from_4bits<uint16_t>(&data[0]),
							get_val_from_4bits<uint16_t>(&data[4]),
							get_val_from_4bits<uint16_t>(&data[8]));

		} else {
			is.setstate(std::ios::failbit);
		} break;
	default:
		is.setstate(std::ios::failbit);
		break;


	}

	if (!is.fail()) col_out = col;

	return is;
}

namespace {
template <typename T>
double ddiff(T v1, T v2)
{
    return std::pow(v1 < v2 ? v2 - v1 : v1 - v2, 2);
}
template <typename T, size_t N>
double color_distance_impl(const std::array<T, N>& c1, const std::array<T, N>& c2)
{
    double sum = 0.0;
    for (auto i = 0u; i < N; ++i) {
        sum += ddiff(c1[i], c2[i]);
    }
    return std::sqrt(sum);
}
}

double color_distance_rgb(const color_t& color1, const color_t& color2)
{
    const auto c1 = color1.get_rgba();
    const auto c2 = color2.get_rgba();
    return color_distance_impl(c1, c2);
}

double color_distance_yuv(const color_t& color1, const color_t& color2)
{
    const auto c1 = color1.get_yuva();
    const auto c2 = color2.get_yuva();
    return color_distance_impl(c1, c2);
}
}
}
