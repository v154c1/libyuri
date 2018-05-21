/*
 * OmxilOutput.cpp
 */

#include "OmxilOutput.h"

#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/frame/raw_audio_frame_types.h"

#include <cassert>
#include <dlfcn.h>

namespace yuri {
namespace omxil {
	
namespace {

std::vector<std::string> base_dlls = {
	{"/opt/vc/lib/libopenmaxil.so"},
	{"libomxil-bellagio.so"},
};

std::vector<std::string> extra_dlls = {
	{"/opt/vc/lib/libbcm_host.so"}
};

std::map<std::string, std::string> role_core_map = {
	{"video_decoder.avc",	"OMX.broadcom.video_decode"},
	{"video_decoder.mpeg2",	"OMX.broadcom.video_decode"},
	{"iv_renderer",			"OMX.broadcom.video_render"},
};

std::map<OMX_ERRORTYPE, std::string> omx_error_map = {
	{OMX_ErrorInsufficientResources,	"OMX_ErrorInsufficientResources"},
	{OMX_ErrorUndefined,				"OMX_ErrorUndefined"},
	{OMX_ErrorInvalidComponentName,		"OMX_ErrorInvalidComponentName"},
	{OMX_ErrorComponentNotFound,		"OMX_ErrorComponentNotFound"},
	{OMX_ErrorInvalidComponent,			"OMX_ErrorInvalidComponent"},
	{OMX_ErrorBadParameter,				"OMX_ErrorBadParameter"},
	{OMX_ErrorOverflow,					"OMX_ErrorOverflow"},
	{OMX_ErrorHardware,					"OMX_ErrorHardware"},
	{OMX_ErrorInvalidState,				"OMX_ErrorInvalidState"},
	{OMX_ErrorStreamCorrupt,			"OMX_ErrorStreamCorrupt"},
	{OMX_ErrorPortsNotCompatible,		"OMX_ErrorPortsNotCompatible"},
	{OMX_ErrorResourcesLost,			"OMX_ErrorResourcesLost"},
	{OMX_ErrorNoMore,					"OMX_ErrorNoMore"},
	{OMX_ErrorVersionMismatch,			"OMX_ErrorVersionMismatch"},
	{OMX_ErrorNotReady,					"OMX_ErrorNotReady"},
	{OMX_ErrorTimeout,					"OMX_ErrorTimeout"},
	{OMX_ErrorSameState,				"OMX_ErrorSameState"},
	{OMX_ErrorResourcesPreempted,		"OMX_ErrorResourcesPreempted"},
	{OMX_ErrorPortUnresponsiveDuringAllocation,	"OMX_ErrorPortUnresponsiveDuringAllocation"},
};

/* TODO: Some more...
"OMX_ErrorPortUnresponsiveDuringDeallocation",
"OMX_ErrorPortUnresponsiveDuringStop", "OMX_ErrorIncorrectStateTransition",
"OMX_ErrorIncorrectStateOperation", "OMX_ErrorUnsupportedSetting",
"OMX_ErrorUnsupportedIndex", "OMX_ErrorBadPortIndex",
"OMX_ErrorPortUnpopulated", "OMX_ErrorComponentSuspended",
"OMX_ErrorDynamicResourcesUnavailable", "OMX_ErrorMbErrorsInFrame",
"OMX_ErrorFormatNotDetected", "OMX_ErrorContentPipeOpenFailed",
"OMX_ErrorContentPipeCreationFailed", "OMX_ErrorSeperateTablesUsed",
"OMX_ErrorTunnelingUnsupported",
"OMX_Error unknown"
**/

std::string error_to_string(OMX_ERRORTYPE error) {
	auto it = omx_error_map.find(error);
	if (it == omx_error_map.end()) return "Unknown error";
	return it->second;
}

std::string core_to_role(std::string core) {
	for (auto f: role_core_map) {
		if (f.second == core) return f.first;
	}
	throw exception::Exception("No rore found.");
}

/*
std::string role_to_core (std::string role) {
	auto it = role_core_map.find(role);
	if (it == role_core_map.end()) throw exception::Exception("No core found.");
	return it->second;
}
*/

}

IOTHREAD_GENERATOR(OmxilOutput)

core::Parameters OmxilOutput::configure() {
	core::Parameters p = IOThread::configure();
	p["name"]["Name of the window"]="";
	return p;
}

OmxilOutput::OmxilOutput(log::Log &log_,core::pwThreadBase parent, const core::Parameters &parameters)
:core::IOThread(log_,parent,0,1,std::string("omxil_output")),
name_("") {
	IOTHREAD_INIT(parameters)
	
	if (!load_libs()) {
		log[log::fatal] << "Failed to initialize omxil library.";
		throw exception::InitializationFailed("Failed to initialize omxil library.");
	}

	// List all components and find correct one for display
	char core_name[OMX_MAX_STRINGNAME_SIZE];
	uint32_t round = 0;
	while(true) {
		OMX_ERRORTYPE omx_error = omx_component_enum_(core_name, OMX_MAX_STRINGNAME_SIZE, round++);
		if(omx_error != OMX_ErrorNone) {
			log[log::fatal] << "Cannot find correct component for rendering.";
			throw exception::InitializationFailed("Cannot find correct component for rendering.");
		}
		try {
			if (core_to_role(core_name) == "iv_renderer") {
				log[log::info] << "Found component: " << core_name << " with apropriate role.";
				break;
			} else {
				log[log::info] << "Found component: " << core_name << ".";
				continue;
			}
		} catch (exception::Exception &e) {
			continue;
		}
	}
}

OmxilOutput::~OmxilOutput() noexcept {
}


void OmxilOutput::run() {
	log[log::info] << "Receiving started";
	while (still_running()) {
	}
	log[log::info] << "Stopping receiver";
}


bool OmxilOutput::set_param(const core::Parameter &param) {
	if (assign_parameters(param)
			(name_, "stream")
			)
		return true;
	return IOThread::set_param(param);
}

bool OmxilOutput::load_libs() {
	// Try to load available libs
	for (auto lib: base_dlls) {
		hbaselib_ = dlopen(lib.c_str(), RTLD_NOW);
        if(hbaselib_) break;
	}
	for (auto lib: extra_dlls) {
		hextralib_ = dlopen(lib.c_str(), RTLD_NOW);
        if(hextralib_) break;
	}
	if (!hbaselib_ || !hextralib_)
		return false;
	// We have them loaded, get functions, first extra ones
	omx_host_init_ = reinterpret_cast<omx_host_init>(dlsym(hextralib_, "bcm_host_init"));
	omx_host_deinit_ = reinterpret_cast<omx_host_deinit>(dlsym(hextralib_, "bcm_host_deinit"));
	if (!omx_host_init_ || !omx_host_deinit_) {
		log[log::fatal] << "Cannot load extra functions.";
        dlclose(hbaselib_);
        dlclose(hextralib_);
        return false;
	}
	// And now core
	omx_init_ = reinterpret_cast<omx_init>(dlsym(hbaselib_, "OMX_Init"));
	omx_deinit_ = reinterpret_cast<omx_deinit>(dlsym(hbaselib_, "OMX_Deinit"));
	omx_get_handle_ = reinterpret_cast<omx_get_handle>(dlsym(hbaselib_, "OMX_GetHandle"));
	omx_free_handle_ = reinterpret_cast<omx_free_handle>(dlsym(hbaselib_, "OMX_FreeHandle"));
	omx_component_enum_ = reinterpret_cast<omx_component_enum>(dlsym(hbaselib_, "OMX_ComponentNameEnum"));
	omx_get_roles_of_component_ = reinterpret_cast<omx_get_roles_of_component>(dlsym(hbaselib_, "OMX_GetRolesOfComponent"));
    if (!omx_init_ || !omx_deinit_ || !omx_get_handle_ || !omx_free_handle_ || !omx_component_enum_ || !omx_get_roles_of_component_) {
		log[log::fatal] << "Cannot load basic functions.";
        dlclose(hbaselib_);
        dlclose(hextralib_);
        return false;
    }
    
    // Try init OMX
    omx_host_init_();
    OMX_ERRORTYPE omx_error = omx_init_();
    if(omx_error != OMX_ErrorNone) {
		log[log::fatal] << "Cannot call omx init function: " << error_to_string(omx_error);
		dlclose(hbaselib_);
        dlclose(hextralib_);
        return false;
	}
	// All good
	return true;
}

}
}

