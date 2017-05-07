/*!
 * @file 		v4l2_controls.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		25.1.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include <errno.h>
#include "v4l2_controls.h"
#include "yuri/core/utils.h"
#include <sys/ioctl.h>
#include <cstring>

namespace yuri {

namespace v4l2 {
namespace controls {

namespace {

	template<class T>
	int32_t get_event_value(const control_state_t& state, const T& value)
	{
		if (value.range_specified()) {
			const auto tgt_range = state.max_value - state.min_value;
			const auto src_range = value.get_max_value() - value.get_min_value();
			return static_cast<int32_t>(static_cast<double>(value.get_value() - value.get_min_value()) * tgt_range / src_range) + state.min_value;
		}
		return value.get_value();
	}

	const std::map<int, std::string> user_controls = {
#ifdef V4L2_CID_BRIGHTNESS
	{V4L2_CID_BRIGHTNESS, "brightness"},
#endif
#ifdef V4L2_CID_CONTRAST
	{V4L2_CID_CONTRAST, "contrast"},
#endif
#ifdef V4L2_CID_SATURATION
	{V4L2_CID_SATURATION, "saturation"},
#endif
#ifdef V4L2_CID_HUE
	{V4L2_CID_HUE, "hue"},
#endif
#ifdef V4L2_CID_AUDIO_VOLUME
	{V4L2_CID_AUDIO_VOLUME, "audio_volume"},
#endif
#ifdef V4L2_CID_AUDIO_BALANCE
	{V4L2_CID_AUDIO_BALANCE, "audio_balance"},
#endif
#ifdef V4L2_CID_AUDIO_BASS
	{V4L2_CID_AUDIO_BASS, "audio_bass"},
#endif
#ifdef V4L2_CID_AUDIO_TREBLE
	{V4L2_CID_AUDIO_TREBLE, "audio_treble"},
#endif
#ifdef V4L2_CID_AUDIO_MUTE
	{V4L2_CID_AUDIO_MUTE, "audio_mute"},
#endif
#ifdef V4L2_CID_AUDIO_LOUDNESS
	{V4L2_CID_AUDIO_LOUDNESS, "audio_loudness"},
#endif
#ifdef V4L2_CID_AUTO_WHITE_BALANCE
	{V4L2_CID_AUTO_WHITE_BALANCE, "auto_white_balance"},
#endif
#ifdef V4L2_CID_DO_WHITE_BALANCE
	{V4L2_CID_DO_WHITE_BALANCE, "do_white_balance"},
#endif
#ifdef V4L2_CID_RED_BALANCE
	{V4L2_CID_RED_BALANCE, "red_balance"},
#endif
#ifdef V4L2_CID_BLUE_BALANCE
	{V4L2_CID_BLUE_BALANCE, "blue_balance"},
#endif
#ifdef V4L2_CID_GAMMA
	{V4L2_CID_GAMMA, "gamma"},
#endif
#ifdef V4L2_CID_EXPOSURE
	{V4L2_CID_EXPOSURE, "exposure"},
#endif
#ifdef V4L2_CID_AUTOGAIN
	{V4L2_CID_AUTOGAIN, "autogain"},
#endif
#ifdef V4L2_CID_GAIN
	{V4L2_CID_GAIN, "gain"},
#endif
#ifdef V4L2_CID_HFLIP
	{V4L2_CID_HFLIP, "hflip"},
#endif
#ifdef V4L2_CID_VFLIP
	{V4L2_CID_VFLIP, "vflip"},
#endif
#ifdef V4L2_CID_POWER_LINE_FREQUENCY
	{V4L2_CID_POWER_LINE_FREQUENCY, "power_line_frequency"},
#endif
#ifdef V4L2_CID_HUE_AUTO
	{V4L2_CID_HUE_AUTO, "hue_auto"},
#endif
#ifdef V4L2_CID_WHITE_BALANCE_TEMPERATURE
	{V4L2_CID_WHITE_BALANCE_TEMPERATURE, "white_ballance_temperature"},
#endif
#ifdef V4L2_CID_SHARPNESS
	{V4L2_CID_SHARPNESS, "sharpness"},
#endif
#ifdef V4L2_CID_BACKLIGHT_COMPENSATION
	{V4L2_CID_BACKLIGHT_COMPENSATION, "backlight_compensation"},
#endif
#ifdef V4L2_CID_CHROMA_AGC
	{V4L2_CID_CHROMA_AGC, "chroma_agc"},
#endif
#ifdef V4L2_CID_COLOR_KILLER
	{V4L2_CID_COLOR_KILLER, "color_killer"},
#endif
#ifdef V4L2_CID_COLORFX
	{V4L2_CID_COLORFX, "color_fx"},
#endif
#ifdef V4L2_CID_AUTOBRIGHTNESS
	{V4L2_CID_AUTOBRIGHTNESS, "auto_brightness"},
#endif
#ifdef V4L2_CID_BAND_STOP_FILTER
	{V4L2_CID_BAND_STOP_FILTER, "band_stop_filter"},
#endif
#ifdef V4L2_CID_ROTATE
	{V4L2_CID_ROTATE, "rotate"},
#endif
#ifdef V4L2_CID_BG_COLOR
	{V4L2_CID_BG_COLOR, "bg_color"},
#endif
#ifdef V4L2_CID_CHROMA_GAIN
	{V4L2_CID_CHROMA_GAIN, "chroma_gain"},
#endif
#ifdef V4L2_CID_ILLUMINATORS_1
	{V4L2_CID_ILLUMINATORS_1, "illuminator"},
#endif
#ifdef V4L2_CID_ILLUMINATORS_2
	{V4L2_CID_ILLUMINATORS_2, "illuminator2"},
#endif
	};

	std::string get_user_control_name(int control)
	{
		auto it = user_controls.find(control);
		if (it == user_controls.end()) return {};
		return it->second;
	}
	int get_user_control_by_name(std::string name)
	{
		for (const auto& ctrl: user_controls) {
			if (iequals(ctrl.second, name)) return ctrl.first;
		}
		return 0;
	}

	bool set_user_control(v4l2_device& dev, uint32_t id, bool value, log::Log& /* log */)
	{
		auto state = dev.is_control_supported(id);
		if (state.supported != control_support_t::supported) {
			return false;
		}
		return dev.set_user_control(id, std::move(state), value?state.max_value:state.min_value);
	}
	bool set_user_control(v4l2_device& dev, uint32_t id, int32_t value, log::Log& /* log */)
	{
		auto state = dev.is_control_supported(id);
		if (state.supported != control_support_t::supported) {
			return false;
		}
		return dev.set_user_control(id, std::move(state), value);
	}


	bool set_user_control(v4l2_device& dev, uint32_t id, const event::pBasicEvent& event, log::Log& /* log */)
	{
		auto state = dev.is_control_supported(id);
		if (state.supported != control_support_t::supported) {
			return false;
		}

		if (event->get_type() == event::event_type_t::boolean_event) {
			return dev.set_user_control(id, std::move(state), event::get_value<event::EventBool>(event));
		}
		if (event->get_type() == event::event_type_t::integer_event) {
			auto value = get_event_value(state, *std::dynamic_pointer_cast<event::EventInt>(event));
			return dev.set_user_control(id, std::move(state), value);
		}
		if (event->get_type() == event::event_type_t::double_event) {
			auto value = get_event_value(state, *std::dynamic_pointer_cast<event::EventDouble>(event));
			return dev.set_user_control(id, std::move(state), value);
		}
		return false;
	}

	const std::map<int, std::string> camera_controls = {
#ifdef V4L2_CID_EXPOSURE_AUTO
	{V4L2_CID_EXPOSURE_AUTO, "exposure_auto"},
#endif
#ifdef V4L2_CID_EXPOSURE_ABSOLUTE
	{V4L2_CID_EXPOSURE_ABSOLUTE, "exposure_absolute"},
#endif
#ifdef V4L2_CID_EXPOSURE_AUTO_PRIORITY
#ifdef V4L2_CID_EXPOSURE_AUTO_PRIORITY
	{V4L2_CID_EXPOSURE_AUTO_PRIORITY, "exposure_auto_priority"},
#endif
#endif
#ifdef V4L2_CID_PAN_RELATIVE
	{V4L2_CID_PAN_RELATIVE, "pan_relative"},
#endif
#ifdef V4L2_CID_TILT_RELATIVE
	{V4L2_CID_TILT_RELATIVE, "tilt_relative"},
#endif
#ifdef V4L2_CID_PAN_RESET
	{V4L2_CID_PAN_RESET, "pan_reset"},
#endif
#ifdef V4L2_CID_TILT_RESET
	{V4L2_CID_TILT_RESET, "tilt_reset"},
#endif
#ifdef V4L2_CID_PAN_ABSOLUTE
	{V4L2_CID_PAN_ABSOLUTE, "pan_absolute"},
#endif
#ifdef V4L2_CID_TILT_ABSOLUTE
	{V4L2_CID_TILT_ABSOLUTE, "tilt_absolute"},
#endif
#ifdef V4L2_CID_FOCUS_ABSOLUTE
	{V4L2_CID_FOCUS_ABSOLUTE, "focus_absolute"},
#endif
#ifdef V4L2_CID_FOCUS_RELATIVE
	{V4L2_CID_FOCUS_RELATIVE, "focus_relative"},
#endif
#ifdef V4L2_CID_FOCUS_AUTO
	{V4L2_CID_FOCUS_AUTO, "focus_auto"},
#endif
#ifdef V4L2_CID_ZOOM_ABSOLUTE
	{V4L2_CID_ZOOM_ABSOLUTE, "zoom_absolute"},
#endif
#ifdef V4L2_CID_ZOOM_RELATIVE
	{V4L2_CID_ZOOM_RELATIVE, "zoom_relative"},
#endif
#ifdef V4L2_CID_ZOOM_CONTINUOUS
	{V4L2_CID_ZOOM_CONTINUOUS, "zoom_continuous"},
#endif
#ifdef V4L2_CID_PRIVACY
	{V4L2_CID_PRIVACY, "privacy"},
#endif
#ifdef V4L2_CID_IRIS_ABSOLUTE
	{V4L2_CID_IRIS_ABSOLUTE, "iris_absolute"},
#endif
#ifdef V4L2_CID_IRIS_RELATIVE
	{V4L2_CID_IRIS_RELATIVE, "iris_relative"},
#endif

#ifdef V4L2_CID_AUTO_EXPOSURE_BIAS
	{V4L2_CID_AUTO_EXPOSURE_BIAS, "exposure_bias_auto"},
#endif
#ifdef V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE
	{V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE, "white_balance_preset_auto"},
#endif
#ifdef	V4L2_CID_WIDE_DYNAMIC_RANGE
	{V4L2_CID_WIDE_DYNAMIC_RANGE, "wide_dynamic_range"},
#endif
#ifdef  V4L2_CID_IMAGE_STABILIZATION
	{V4L2_CID_IMAGE_STABILIZATION, "image_stabilization"},
#endif
#ifdef V4L2_CID_ISO_SENSITIVITY
	{V4L2_CID_ISO_SENSITIVITY, "iso_sensitivity"},
#endif
#ifdef V4L2_CID_ISO_SENSITIVITY_AUTO
	{V4L2_CID_ISO_SENSITIVITY_AUTO, "iso_sensitivity_auto"},
#endif
#ifdef V4L2_CID_EXPOSURE_METERING
	{V4L2_CID_EXPOSURE_METERING, "exposure_metering"},
#endif
#ifdef V4L2_CID_SCENE_MODE
	{V4L2_CID_SCENE_MODE, "scene_mode"},
#endif
#ifdef V4L2_CID_3A_LOCK
	{V4L2_CID_3A_LOCK, "3a_lock"}
#endif
	};

	std::string get_camera_control_name(int control)
	{
		auto it = camera_controls.find(control);
		if (it == camera_controls.end()) return {};
		return it->second;
	}
	int get_camera_control_by_name(std::string name)
	{
		for (const auto& ctrl: camera_controls) {
			if (iequals(ctrl.second, name)) return ctrl.first;
		}
		return 0;
	}


	bool set_camera_control(v4l2_device& dev, uint32_t id, bool value, log::Log& /* log */)
	{
		auto state = dev.is_control_supported(id);
		if (state.supported != control_support_t::supported) {
			return false;
		}
		return dev.set_camera_control(id, std::move(state), value?state.max_value:state.min_value);
	}
	bool set_camera_control(v4l2_device& dev, uint32_t id, int32_t value, log::Log& /* log */)
	{
		auto state = dev.is_control_supported(id);
		if (state.supported != control_support_t::supported) {
			return false;
		}
		return dev.set_camera_control(id, std::move(state), value);
	}


	bool set_camera_control(v4l2_device& dev, uint32_t id, const event::pBasicEvent& event, log::Log& /* log */)
	{
		auto state = dev.is_control_supported(id);
		if (state.supported != control_support_t::supported) {
			return false;
		}

		if (event->get_type() == event::event_type_t::boolean_event) {
			return dev.set_camera_control(id, std::move(state), event::get_value<event::EventBool>(event));
		}
		if (event->get_type() == event::event_type_t::integer_event) {
			auto value = get_event_value(state, *std::dynamic_pointer_cast<event::EventInt>(event));
			return dev.set_camera_control(id, std::move(state), value);
		}
		if (event->get_type() == event::event_type_t::double_event) {
			auto value = get_event_value(state, *std::dynamic_pointer_cast<event::EventDouble>(event));
			return dev.set_camera_control(id, std::move(state), value);
		}
		return false;
		//return set_control_impl(fd, id, std::move(state), value, log);
	}


	template<class T>
	bool set_control_generic(v4l2_device& dev, int id, const T& value, log::Log& log)
	{
		if (contains(user_controls, id)) return set_user_control(dev, id, value, log);
		if (contains(camera_controls, id)) return set_camera_control(dev, id, value, log);
		return false;
	}

	template<class T>
	bool set_control_generic(v4l2_device& dev, const std::string& name, const T& value, log::Log& log)
	{
		auto id = get_user_control_by_name(name);
		if (id) return set_user_control(dev, id, value, log);
		auto cid = get_camera_control_by_name(name);
		if (cid) return set_camera_control(dev, id, value, log);
		return false;
	}
}


std::vector<control_info> get_control_list(v4l2_device& dev, log::Log& /* log */)
{
	std::vector<control_info> ctrls;
	for (const auto& c: user_controls) {
		auto state = dev.is_user_control_supported(c.first);
		if (state.supported == control_support_t::supported) {
			ctrls.push_back({c.first, state.name, c.second, state.value, state.min_value, state.max_value});
		}
	}
	for (const auto& c: camera_controls) {
		auto state = dev.is_camera_control_supported(c.first);
		if (state.supported == control_support_t::supported) {
			ctrls.push_back({c.first, state.name, c.second, state.value, state.min_value, state.max_value});
		}
	}
	return ctrls;
}


std::string get_control_name(int control)
{
	auto n = get_user_control_name(control);
	if (!n.empty()) return n;
	return get_camera_control_name(control);
}
int get_control_by_name(std::string name)
{
	if (auto id = get_user_control_by_name(name)) {
		return id;
	}
	return get_camera_control_by_name(name);
}


bool set_control(v4l2_device& dev, int id, bool value, log::Log& log)
{
	return set_control_generic(dev, id, value, log);
}
bool set_control(v4l2_device& dev, int id, int32_t value, log::Log& log)
{
	return set_control_generic(dev, id, value, log);
}
bool set_control(v4l2_device& dev, int id, const event::pBasicEvent& value, log::Log& log)
{
	return set_control_generic(dev, id, value, log);
}

bool set_control(v4l2_device& dev, const std::string& name, bool value, log::Log& log)
{
	return set_control_generic(dev, name, value, log);
}
bool set_control(v4l2_device& dev, const std::string& name, int32_t value, log::Log& log)
{
	return set_control_generic(dev, name, value, log);
}
bool set_control(v4l2_device& dev, const std::string& name, const event::pBasicEvent& value, log::Log& log)
{
	return set_control_generic(dev, name, value, log);
}





}
}
}
