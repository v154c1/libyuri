/*!
 * @file 		HikVision.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		28.02.2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "HikVision.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include <cstring>

namespace yuri {
namespace hikvision {

IOTHREAD_GENERATOR(HikVision)

MODULE_REGISTRATION_BEGIN("hikvision")
REGISTER_IOTHREAD("hikvision", HikVision)
MODULE_REGISTRATION_END()

core::Parameters HikVision::configure()
{
    core::Parameters p = core::IOThread::configure();
    p.set_description("Interface to HikVision DVR systems");
    p["address"]["IP address of the remote system"] = "127.0.0.1";
    p["port"]["Port of the remote system"]          = 8000;
    p["username"]["Username"]                       = "admin";
    p["password"]["password"]                       = "12345";
    p["channel"]["Channel"]                         = 0;
    p["decode"]["Decode images"]                    = false;
    return p;
}

HikVision::HikVision(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : core::IOThread(log_, parent, 0, 1, std::string("hikvision")),
      address_{ "127.0.0.1" },
      port_{ 8000 },
      username_{ "admin" },
      password_{ "12345" },
      channel_{ 0 },
      user_id_{ -1 },
      decode_{ false },
      port_id_{ -1 }
{
    IOTHREAD_INIT(parameters)
    NET_DVR_Init();
    const auto uiVersion = NET_DVR_GetSDKBuildVersion();
    log[log::info] << "Started HikVision SDK, version " << ((uiVersion & 0xff000000) >> 24) << "." << ((uiVersion & 0x00ff0000) >> 16) << "."
                   << ((uiVersion & 0x0000ff00) >> 8) << "." << (uiVersion & 0xff);
    //	NET_DVR_SetLogToFile(3, "./sdkLog");
    NET_DVR_DEVICEINFO_V30 dev_info;
    std::memset(reinterpret_cast<char*>(&dev_info), 0, sizeof(NET_DVR_DEVICEINFO_V30));
    //	log[log::info] << "connecting to " << address_ << ":" << port_ << " as " << username_ << ":" << password_;
    user_id_ = NET_DVR_Login_V30(&address_[0], port_, &username_[0], &password_[0], &dev_info);
    if (user_id_ < 0) {
        log[log::fatal] << "Failed to login";
        throw exception::InitializationFailed("Failed to login into the device");
    }
    log[log::info] << "Connected to device with SN: " << dev_info.sSerialNumber << " with " << static_cast<int>(dev_info.byChanNum)
                   << " channels starting at index " << static_cast<int>(dev_info.byStartChan);
}

HikVision::~HikVision() noexcept
{
    NET_DVR_Logout_V30(user_id_);
    NET_DVR_Cleanup();
}

namespace {

void hik_callback_data(LONG /* lRealHandle */, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* dwUser)
{
    auto* h = reinterpret_cast<HikVision*>(dwUser);
    h->data_read(dwDataType, pBuffer, dwBufSize);
}

void hik_callback_image(int /* nPort */, char* pBuf, int nSize, FRAME_INFO* pFrameInfo, void* nUser, int)
{
    auto* h = reinterpret_cast<HikVision*>(nUser);
    h->image_ready(pBuf, nSize, pFrameInfo);
}
// std::string
int hik_error()
{
    return NET_DVR_GetLastError();
}
}
void HikVision::run()
{
    NET_DVR_CLIENTINFO client_info_;
    std::memset(reinterpret_cast<char*>(&client_info_), 0, sizeof(NET_DVR_CLIENTINFO));
    client_info_.lChannel     = channel_;
    client_info_.lLinkMode    = 0;
    client_info_.hPlayWnd     = 0;
    client_info_.sMultiCastIP = nullptr;
    auto real_handle          = NET_DVR_RealPlay_V30(user_id_, &client_info_, hik_callback_data, this, 0);
    if (real_handle < 0) {
        log[log::info] << "Failed to start playback for channel " << channel_ << ": " << hik_error();
        return;
    }
    while (still_running()) {
        sleep(get_latency());
    }
    if (port_id_ >= 0) {
        PlayM4_Stop(port_id_);
        PlayM4_CloseStream(port_id_);
        PlayM4_FreePort(port_id_);
    }
    NET_DVR_StopRealPlay(real_handle);
}

void HikVision::data_read(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize)
{

    if (!decode_) {
        auto frame = core::CompressedVideoFrame::create_empty(core::compressed_frame::h264, resolution_t{ 0, 0 }, pBuffer, dwBufSize);
        push_frame(0, std::move(frame));
        return;
    }
    switch (dwDataType) {
    case NET_DVR_SYSHEAD:
        if (!PlayM4_GetPort(&port_id_)) {
            log[log::warning] << "Failed to get decoder port";
            return;
        }
        if (!PlayM4_SetStreamOpenMode(port_id_, STREAME_REALTIME)) {
            log[log::warning] << "Failed to set stream mode";
            return;
        }
        if (!PlayM4_OpenStream(port_id_, pBuffer, dwBufSize, 1048675)) {
            log[log::warning] << "Failed to open stream";
            return;
        }
        PlayM4_SetDecCBStream(port_id_, 1);
        PlayM4_SetDecCallBackMend(port_id_, hik_callback_image, this);
        if (!PlayM4_Play(port_id_, 0)) {
            log[log::warning] << "Failed to start playback";
            return;
        }
        log[log::info] << "Decoder initialized";
        break;
    case NET_DVR_STREAMDATA:
        if (!PlayM4_InputData(port_id_, pBuffer, dwBufSize)) {
            log[log::warning] << "Failed to process incomming data";
        }
        break;
    default:
        log[log::info] << "Read " << dwBufSize << " bytes of unsupported type " << dwDataType;
    }
}

void HikVision::image_ready(char* pBuf, int nSize, FRAME_INFO* pFrameInfo)
{
    switch (pFrameInfo->nType) {
    case T_YV12: {
        auto frame = core::RawVideoFrame::create_empty(
            core::raw_format::yuv420p, resolution_t{ static_cast<dimension_t>(pFrameInfo->nWidth), static_cast<dimension_t>(pFrameInfo->nHeight) });
        if (static_cast<size_t>(nSize) < PLANE_SIZE(frame, 0) + PLANE_SIZE(frame, 1) + PLANE_SIZE(frame, 2)) {
            log[log::info] << "Not enought data!";
            return;
        }

        std::copy(pBuf, pBuf + PLANE_SIZE(frame, 0), PLANE_DATA(frame, 0).begin());
        auto idx = PLANE_SIZE(frame, 0);
        std::copy(pBuf + idx, pBuf + idx + PLANE_SIZE(frame, 2), PLANE_DATA(frame, 2).begin());
        idx += PLANE_SIZE(frame, 2);
        std::copy(pBuf + idx, pBuf + idx + PLANE_SIZE(frame, 1), PLANE_DATA(frame, 1).begin());

        push_frame(0, std::move(frame));
    }; break;
    default:
        log[log::info] << "Unsupported type";
    }
}

bool HikVision::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)(address_, "address")(port_, "port")(username_, "username")(password_, "password")(channel_, "channel")(decode_, "decode"))
        return true;
    return core::IOThread::set_param(param);
}

} /* namespace hikvision */
} /* namespace yuri */
