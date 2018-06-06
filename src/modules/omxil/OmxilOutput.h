/*
 * OmxilOutput.h
  */

#ifndef OMXILOUTPUT_H_
#define OMXILOUTPUT_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventConsumer.h"

#include "OMX_Core.h"
#include "OMX_Index.h"
#include "OMX_Component.h"
#include "OMX_Video.h"
#include "OMX_Broadcom.h"

namespace yuri {
namespace omxil {
	
typedef OMX_ERRORTYPE (*omx_init)(void);
typedef OMX_ERRORTYPE (*omx_deinit)(void);
typedef OMX_ERRORTYPE (*omx_get_handle)(OMX_HANDLETYPE *, OMX_STRING, OMX_PTR, OMX_CALLBACKTYPE *);
typedef OMX_ERRORTYPE (*omx_free_handle)(OMX_HANDLETYPE);
typedef OMX_ERRORTYPE (*omx_component_enum)(OMX_STRING, OMX_U32, OMX_U32);
typedef OMX_ERRORTYPE (*omx_get_roles_of_component)(OMX_STRING, OMX_U32 *, OMX_U8 **);

typedef void (*omx_host_init)(void);
typedef void (*omx_host_deinit)(void);

class OmxilOutput:public core::IOThread {
public:
	IOTHREAD_GENERATOR_DECLARATION
	OmxilOutput(log::Log &log_,core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~OmxilOutput() noexcept;
	virtual void run() override;
	static core::Parameters configure();
private:
	virtual bool set_param(const core::Parameter &param) override;
	
	bool load_libs();

	std::string name_;
	
	void *hbaselib_;
	void *hextralib_;
	
	omx_init omx_init_;
	omx_deinit omx_deinit_;
	omx_get_handle omx_get_handle_;
	omx_free_handle omx_free_handle_;
	omx_component_enum omx_component_enum_;
	omx_get_roles_of_component omx_get_roles_of_component_;
	
	omx_host_init omx_host_init_;
	omx_host_deinit omx_host_deinit_;
};

}
}

#endif /* OMXILOUTPUT_H_ */
