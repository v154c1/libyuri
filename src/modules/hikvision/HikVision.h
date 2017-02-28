/*!
 * @file 		HikVision.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		28.02.2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef HIKVISION_H_
#define HIKVISION_H_

#include "yuri/core/thread/IOThread.h"
#include "HCNetSDK.h"
#include "PlayM4.h"

namespace yuri {
namespace hikvision {

class HikVision : public core::IOThread {
public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    HikVision(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~HikVision() noexcept;

    void data_read(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize);
    void image_ready(char* pBuf, int nSize, FRAME_INFO* pFrameInfo);

private:
    virtual void run() override;
    virtual bool set_param(const core::Parameter& param) override;

    std::string address_;
    int         port_;
    std::string username_;
    std::string password_;
    int         channel_;

    LONG user_id_;
    bool decode_;
    LONG port_id_;
};

} /* namespace hikvision */
} /* namespace yuri */
#endif /* HIKVISION_H_ */
