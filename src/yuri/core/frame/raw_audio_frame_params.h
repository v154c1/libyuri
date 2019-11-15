/*!
 * @file 		raw_audio_frame_params.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		21.10.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef RAW_AUDIO_FRAME_PARAMS_H_
#define RAW_AUDIO_FRAME_PARAMS_H_
#include "yuri/core/utils/new_types.h"
#include <vector>
#include <map>

namespace yuri {
namespace core {
namespace raw_audio_format {
struct raw_audio_format_t;

EXPORT const raw_audio_format_t &get_format_info(format_t format);
EXPORT bool add_format(const raw_audio_format_t &);
EXPORT format_t new_user_format();
EXPORT format_t parse_format(const std::string& name);
EXPORT const std::string& get_format_name(format_t format);
using format_info_map_t = std::map<format_t, raw_audio_format_t>;


struct formats {
	EXPORT format_info_map_t::const_iterator begin() const;
	EXPORT format_info_map_t::const_iterator end() const;
};


struct raw_audio_format_t {
	EXPORT  raw_audio_format_t(format_t format, std::string long_name,
               std::vector<std::string> short_names, size_t bits_per_sample,
               bool little_endian = true, bool planar = false)
      : format(format), name(long_name), short_names(short_names),
        bits_per_sample(bits_per_sample),little_endian(little_endian), planar(planar) {}
	EXPORT ~raw_audio_format_t() noexcept {}
  format_t format;
  std::string name;
  std::vector<std::string> short_names;
  size_t	bits_per_sample;
  bool little_endian;
  bool planar;

};

inline const std::string& get_format_name(format_t format) { return get_format_info(format).name; }

}
}
}


#endif /* RAW_AUDIO_FRAME_PARAMS_H_ */
