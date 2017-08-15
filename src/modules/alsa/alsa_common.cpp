/*!
 * @file 		alsa_common.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>, Jiri Melnikov <jiri@melnikoff.org>
 * @date		1.10.2017
 * @copyright
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

namespace {
using namespace core::raw_audio_format;

std::map<format_t, snd_pcm_format_t> yuri_to_alsa_formats = {
		{unsigned_8bit, 	SND_PCM_FORMAT_U8},
		{unsigned_16bit, 	SND_PCM_FORMAT_U16_LE},
		{signed_16bit, 		SND_PCM_FORMAT_S16_LE},
		{unsigned_24bit, 	SND_PCM_FORMAT_U24_3LE},
		{signed_24bit, 		SND_PCM_FORMAT_S24_3LE},
		{unsigned_32bit, 	SND_PCM_FORMAT_U32_LE},
		{signed_32bit, 		SND_PCM_FORMAT_S32_LE},
		{float_32bit, 		SND_PCM_FORMAT_FLOAT_LE},

		{unsigned_16bit_be,	SND_PCM_FORMAT_U16_BE},
		{signed_16bit_be,	SND_PCM_FORMAT_S16_BE},
		{unsigned_24bit_be, SND_PCM_FORMAT_U24_3BE},
		{signed_24bit_be, 	SND_PCM_FORMAT_S24_3BE},
		{unsigned_32bit_be, SND_PCM_FORMAT_U32_BE},
		{signed_32bit_be,	SND_PCM_FORMAT_S32_BE},
		{float_32bit_be,	SND_PCM_FORMAT_FLOAT_BE},
};

snd_pcm_format_t get_alsa_format(format_t fmt) {
	auto it = yuri_to_alsa_formats.find(fmt);
	if (it == yuri_to_alsa_formats.end()) return SND_PCM_FORMAT_UNKNOWN;
	return it->second;
}

}

