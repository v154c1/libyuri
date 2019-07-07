/*!
 * @file 		DeckLinkBase.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		22.9.2011
 * @date		21.11.2013
 * @copyright	CESNET, z.s.p.o, 2011 - 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */


#include "DeckLinkBase.h"
//#include <boost/assign.hpp>
//#include <boost/algorithm/string.hpp>
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/utils.h"
#include <cstdlib>
namespace yuri {

namespace decklink {

namespace {
using namespace yuri::core::raw_format;
struct compare_insensitive
{
	bool operator()(const std::string& a, const std::string& b) const
	{
		return iless(a,b);
	}
};

std::map<std::string, BMDDisplayMode, compare_insensitive>
mode_strings = {
// SD Modes
		{"pal",			bmdModePAL},
		{"ntsc",		bmdModeNTSC},
		{"ntsc2398",	bmdModeNTSC2398},
		{"ntscp",		bmdModeNTSCp},
		{"palp",		bmdModePALp},
// Progressive HD 1080 modes
		{"1080p2398",	bmdModeHD1080p2398},
		{"1080p24",		bmdModeHD1080p24},
		{"1080p25",		bmdModeHD1080p25},
		{"1080p2997",	bmdModeHD1080p2997},
		{"1080p30",		bmdModeHD1080p30},
		{"1080p50",		bmdModeHD1080p50},
		{"1080p5994",	bmdModeHD1080p5994},
		{"1080p60",		bmdModeHD1080p6000},
#ifdef DECKLINK_API_11
        {"1080p9550",	bmdModeHD1080p9590},
        {"1080p96",		bmdModeHD1080p96},
        {"1080p100",		bmdModeHD1080p100},
        {"1080p11988",	bmdModeHD1080p11988},
        {"1080p120",	bmdModeHD1080p120},
#endif
// Progressive 720p modes
		{"720p50",		bmdModeHD720p50},
		{"720p5994",	bmdModeHD720p5994},
		{"720p60",		bmdModeHD720p60},
// Interlaced HD 1080 Modes
		{"1080i50",		bmdModeHD1080i50},
		{"1080i5994",	bmdModeHD1080i5994},
		{"1080i60",		bmdModeHD1080i6000},
// PsF modes
		{"1080p24PsF",	bmdModeHD1080p24},
		{"1080p2398PsF",bmdModeHD1080p2398},
// 2k Modes
		{"2k24",		bmdMode2k24},
		{"2k2398",		bmdMode2k2398},
		{"2k25",		bmdMode2k25},
// 4k Modes
		{"4k2398",		bmdMode4K2160p2398},
		{"4k24",		bmdMode4K2160p24},
		{"4k25",		bmdMode4K2160p25},
		{"4k2997",		bmdMode4K2160p2997},
		{"4k30",		bmdMode4K2160p30},
        {"4k4795",		bmdMode4K2160p4795},
        {"4k48",		bmdMode4K2160p48},
        {"4k50",		bmdMode4K2160p50},
        {"4k5994",		bmdMode4K2160p5994},
        {"4k60",		bmdMode4K2160p60},
#ifdef DECKLINK_API_11
        {"4k9590",		bmdMode4K2160p9590},
        {"4k96",		bmdMode4K2160p96},
        {"4k100",		bmdMode4K2160p100},
        {"4k11988",		bmdMode4K2160p11988},
        {"4k120",		bmdMode4K2160p120},
// 8k modes
        {"8k2398",		bmdMode8K4320p2398},
        {"8k24",		bmdMode8K4320p24},
        {"8k25",		bmdMode8K4320p25},
        {"8k2997",		bmdMode8K4320p2997},
        {"8k30",		bmdMode8K4320p30},
        {"8k4795",		bmdMode8K4320p4795},
        {"8k48",		bmdMode8K4320p48},
        {"8k50",		bmdMode8K4320p50},
        {"8k5994",		bmdMode8K4320p5994},
        {"8k60",		bmdMode8K4320p60},
// PC modes
        {"vga",		    bmdMode640x480p60},
        {"svga",		bmdMode800x600p60},
        {"wxga+p50",	bmdMode1440x900p50},
        {"wxga+p60",	bmdMode1440x900p60},
        {"uxgap50",	    bmdMode1600x1200p50},
        {"uxgap60",	    bmdMode1600x1200p60},
        {"wuxgap50",	bmdMode1920x1200p50},
        {"wuxgap60",	bmdMode1920x1200p60},
        {"qhdp50",	    bmdMode2560x1440p50},
        {"qhdp60",	    bmdMode2560x1440p60},
        {"wqxgap50",	bmdMode2560x1600p50},
        {"wqxgap60",	bmdMode2560x1600p60},





#endif
};

std::map<std::string, BMDPixelFormat, compare_insensitive>
pixfmt_strings = {
		{"yuv", 		bmdFormat8BitYUV},
		{"v210", 		bmdFormat10BitYUV},
		{"argb",		bmdFormat8BitARGB},
		{"bgra",		bmdFormat8BitBGRA},
		{"r210",		bmdFormat10BitRGB},
};

std::map<std::string, BMDVideoConnection, compare_insensitive>
connection_strings = {
		{"SDI", 		bmdVideoConnectionSDI},
	    {"HDMI",		bmdVideoConnectionHDMI},
	    {"Optical SDI",	bmdVideoConnectionOpticalSDI},
	    {"Component",	bmdVideoConnectionComponent},
	    {"Composite",	bmdVideoConnectionComposite},
	    {"SVideo",		bmdVideoConnectionSVideo},
};

std::map<BMDPixelFormat, yuri::format_t> pixel_format_map = {
		{bmdFormat8BitYUV,	uyvy422},
		{bmdFormat10BitYUV,	yuv422_v210},
		{bmdFormat8BitARGB,	argb32},
		{bmdFormat8BitBGRA,	bgra32},
		{bmdFormat10BitRGB,	rgb_r10k},
};

std::map<std::string, std::string>
progresive_to_psf = {
{"1080p24", "1080p24PsF"},
{"1080p2398", "1080p2398PsF"},
};

template<typename T, typename C>
std::string make_string_list(const std::map<std::string, T, C>& data, const std::string& separator = ", ")
{
	std::string out;
	for (auto& val: data) {
		if (!out.empty()) out+=separator;
		out+=val.first;
	}
	return out;
}

}
core::Parameters DeckLinkBase::configure()
{
	core::Parameters p = IOThread::configure();
	p.set_description("DeckLink SDK Base");
	p["device"]["Index of device to use"]=0;
	p["format"]["Video format. Accepted values: "+make_string_list(mode_strings)]="1080p25";
	p["audio"]["Enable audio"]=false;
	p["pixel_format"]["Select pixel format. Accepted values are: "+make_string_list(pixfmt_strings)]="yuv";
	p["audio_channels"]["Number of audio channels to process, supported are 2, 8 or 16 channels"]=2;
	p["connection"]["Connection (input/output). Accepted values are: "+make_string_list(connection_strings)]="SDI";
	return p;
}

BMDDisplayMode parse_format(const std::string& fmt)
{
	auto it = mode_strings.find(fmt);
	if (it != mode_strings.end()) return it->second;

	return bmdModeUnknown;
}

std::string bm_mode_to_yuri(BMDDisplayMode fmt)
{
	for (const auto& m: mode_strings) {
		if (m.second == fmt) return m.first;
	}
	return "none";
}

BMDVideoConnection parse_connection(const std::string& fmt)
{
	if (connection_strings.count(fmt))
		return connection_strings[fmt];
	else return bmdVideoConnectionHDMI;
}
std::string bm_connection_to_yuri(BMDVideoConnection fmt)
{
	for (auto f: connection_strings) {
		if (f.second == fmt)
			return f.first;
	}
	return "none";
}
BMDPixelFormat convert_yuri_to_bm(format_t fmt)
{
	for (auto f: pixel_format_map) {
		if (f.second == fmt) return f.first;
	}
	return 0;
}
format_t convert_bm_to_yuri(BMDPixelFormat fmt)
{
	auto it = pixel_format_map.find(fmt);
	if (it == pixel_format_map.end()) return 0;
	return it->second;
}
const std::string bmerr(HRESULT res)
{
	switch (HRESULT_CODE(res)) {
		case S_OK: return "OK";
		case S_FALSE: return "False";
	}
	switch (res) {
		case E_FAIL: return "Failed";
		case E_ACCESSDENIED: return "Access denied";
		case E_OUTOFMEMORY: return "OUTOFMEMORY";
		case E_INVALIDARG:return "Invalid argument";
		default: return "Unknown";
	}
}
BMDDisplayMode get_next_format(BMDDisplayMode fmt)
{
	auto it = mode_strings.begin();
	for (; it != mode_strings.end(); ++it) {
		if (it->second == fmt) {
			++it;break;
		}
	}
	if (it == mode_strings.end())
		return mode_strings.begin()->second;
	return it->second;
}

namespace {

void push_cfg_connections(core::InputDeviceInfo& device, core::InputDeviceConfig&& cfg, const std::vector<std::string> conns)
{
	if (conns.empty()) {
		device.configurations.push_back(std::move(cfg));
	} else {
		for (const auto& con: conns) {
			auto cfg2 = cfg;
			cfg2.params["connection"]=con;
			device.configurations.push_back(std::move(cfg2));
		}
	}
}

core::InputDeviceInfo enum_input_device(IDeckLink* dev, uint16_t device_index)
{
	core::InputDeviceInfo device;
	device.main_param_order={"device","connection", "format", "stereo", "pixel_format"};
	const char * device_name = nullptr;
	dev->GetDisplayName(&device_name);
	if (device_name) {
		device.device_name=device_name;
		std::free (const_cast<char*>(device_name));
	} else {
		device.device_name="Unknown Decklink device";
	}

    IDeckLinkInput *input = nullptr;
#ifdef DECKLINK_API_11
	IDeckLinkProfileAttributes *attr = nullptr;
    dev->QueryInterface(IID_IDeckLinkProfileAttributes, reinterpret_cast<void**>(&attr));

    if (dev->QueryInterface(IID_IDeckLinkInput,reinterpret_cast<void**>(&input))!=S_OK) {
        // Not an input device
        return device;
    }
#else
    IDeckLinkAttributes *attr = nullptr;
	dev->QueryInterface(IID_IDeckLinkAttributes,reinterpret_cast<void**>(&attr));

	if (dev->QueryInterface(IID_IDeckLinkInput,reinterpret_cast<void**>(&input))!=S_OK) {
		// Not an input device
		return device;
	}
#endif
	bool detection_supported;
	if(attr->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &detection_supported) != S_OK) {
		detection_supported = false;
	} else {
		if (!detection_supported) {
			detection_supported = false;
		} else {
			detection_supported = true;
		}
	}
	std::vector<std::string> connections;
	int64_t connection_mask;
	attr->GetInt(bmdDeckLinkConfigVideoInputConnection,&connection_mask);
	// It would be nice to detect supported formats based on connection,
	// but there doesn't seem to be a way to do it in the SDK now...
	for (auto f: connection_strings) {
			if (connection_mask & f.second) {
				connections.push_back(f.first);
			}
	}

	IDeckLinkDisplayModeIterator* mode_iter = nullptr;
	if (input->GetDisplayModeIterator(&mode_iter) != S_OK) {
		return device;
	}
	IDeckLinkDisplayMode* mode = nullptr;
	while (mode_iter->Next(&mode) == S_OK) {
		core::InputDeviceConfig cfg;
		cfg.params["device"]=device_index;
		auto m = mode->GetDisplayMode();
		cfg.params["format"]=bm_mode_to_yuri(m);
		for (const auto& pixfmt: pixel_format_map) {
#ifdef DECKLINK_API_11
		    bool supported = false;
		    // FIXME - connection enumeration (?)
            if (input->DoesSupportVideoMode(0, m, pixfmt.first, bmdVideoInputFlagDefault, &supported) == S_OK) {
                if (supported) {
                    try {
                        const auto& fi = get_format_info(pixfmt.second);
                        if (fi.short_names.empty()) continue;
                        auto cfg2 = cfg;
                        cfg2.params["pixel_format"]=fi.short_names[0];
                        cfg2.params["stereo"]=false;
// FIXME missing 3D support
//                        if (res_mode->GetFlags() & bmdDisplayModeSupports3D) {
//                            auto cfg3 = cfg2;
//                            cfg3.params["stereo"]=true;
//                            push_cfg_connections(device, std::move(cfg3), connections);
//                        }
                        push_cfg_connections(device, std::move(cfg2), connections);
                    }
                    catch (...) {

                    }
                }
            }
#else
			BMDDisplayModeSupport sup;
			IDeckLinkDisplayMode* res_mode = nullptr;
			if (input->DoesSupportVideoMode(m, pixfmt.first, bmdVideoInputFlagDefault, &sup, &res_mode) == S_OK) {
				if (sup == bmdDisplayModeSupported || sup == bmdDisplayModeSupportedWithConversion) {
					try {
						const auto& fi = get_format_info(pixfmt.second);
						if (fi.short_names.empty()) continue;
						auto cfg2 = cfg;
						cfg2.params["pixel_format"]=fi.short_names[0];
						cfg2.params["stereo"]=false;

						if (res_mode->GetFlags() & bmdDisplayModeSupports3D) {
							auto cfg3 = cfg2;
							cfg3.params["stereo"]=true;
							push_cfg_connections(device, std::move(cfg3), connections);
						}
						push_cfg_connections(device, std::move(cfg2), connections);
					}
					catch (...) {

					}
				}
			}
#endif
		}

	}

	return device;
}
}

std::vector<core::InputDeviceInfo> DeckLinkBase::enumerate_inputs()
{
	std::vector<core::InputDeviceInfo> devices;
	IDeckLinkIterator *iter = CreateDeckLinkIteratorInstance();
	if (!iter) return devices;
	uint16_t idx = 0;
	IDeckLink* dev;
	while (iter->Next(&dev)==S_OK) {
		if (!dev) {
			continue;
		}
		devices.push_back(enum_input_device(dev, idx));

		dev->Release();
		idx++;
	}

	return devices;
}

DeckLinkBase::DeckLinkBase(const log::Log &log_, core::pwThreadBase parent, position_t inp, position_t outp, const std::string& name)
	:IOThread(log_,parent,inp,outp,name),device(0),device_index(0),connection(bmdVideoConnectionHDMI),
	 mode(bmdModeHD1080p25),pixel_format(bmdFormat8BitYUV),
	 audio_sample_rate(bmdAudioSampleRate48kHz),audio_sample_type(bmdAudioSampleType16bitInteger),
	 audio_channels(2),audio_enabled(false),actual_format_is_psf(false)
{

}

DeckLinkBase::~DeckLinkBase() noexcept
{

}

bool DeckLinkBase::set_param(const core::Parameter &p)
{
	if (p.get_name() == "device") {
		device_index = p.get<uint16_t>();
	} else if (p.get_name() ==  "connection") {
		connection=parse_connection(p.get<std::string>());
	} else if (p.get_name() == "format") {
		BMDDisplayMode m = parse_format(p.get<std::string>());
		if (m == bmdModeUnknown) {
			mode = bmdModeHD1080p25;
			log[log::error] << "Failed to parse format "<< p.get<std::string>()<<", falling back "
					"to 1080p25";
			actual_format_is_psf = false;
		} else {
			mode = m;
			actual_format_is_psf = is_psf(p.get<std::string>());
		}
		log[log::info] << "Using " << get_mode_name(mode) << " (parsed from " << p.get<std::string>() <<")";
	} else if (p.get_name() ==  "audio") {
		audio_enabled=p.get<bool>();
	} else if (p.get_name() =="pixel_format") {
		if (pixfmt_strings.count(p.get<std::string>())) {
			pixel_format=pixfmt_strings[p.get<std::string>()];
			log[log::debug] << "Pixelformat set to " << p.get<std::string>();
		}
		else {
			log[log::warning] << "Unknown pixel format specified (" << p.get<std::string>()
			<< ". Using default 8bit YUV 4:2:2.";
			pixel_format=bmdFormat8BitYUV;
		}
	} else if (p.get_name() == "audio_channels") {
		audio_channels=p.get<size_t>();
	} else return IOThread::set_param(p);
	return true;
}

bool DeckLinkBase::init_decklink()
{
	IDeckLinkIterator *iter = CreateDeckLinkIteratorInstance();
	if (!iter) return false;
	uint16_t idx = 0;
	while (iter->Next(&device)==S_OK) {
		//if (!device) continue;
		if (idx==device_index) {
			break;
		}
		if (!device) {
			log[log::warning] << "Did not receive valid device for index " << idx;
		} else {
			device->Release();
			device = 0;
			log[log::debug] << "Skipping device " << idx;
		}
		idx++;
	}
	if (device_index != idx) {
		log[log::fatal] << "There is no device with index " << device_index << " connected to he system.";
		return false;
	}
	if (!device) {
		log[log::fatal] << "Device not allocated properly!";
		return false;
	}
	const char * device_name = 0;
	device->GetDisplayName(&device_name);
	if (device_name) {
		log[log::info] << "Using blackmagic device: " << device_name;
		std::free (const_cast<char*>(device_name)); // Calling free becaus it's allocated inside decklink api using malloc
	}
	return true;
}
std::string DeckLinkBase::get_mode_name(BMDDisplayMode mode, bool psf)
{
	for (auto it: mode_strings) {
		if (it.second == mode) {
			if (psf) {
				if (progresive_to_psf.count(it.first)) {
					return progresive_to_psf[it.first];
				}
			}
			return it.first;
		}
	}
	return std::string();
}
bool DeckLinkBase::is_psf(const std::string& name)
{
	for (auto it: progresive_to_psf) {
		if (it.second == name) return true;
	}
	return false;
}
}

}
