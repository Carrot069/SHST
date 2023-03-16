#include "RealCamera.h"

/* System */
#include <algorithm>
#include <chrono>
#include <cstring>
#include <map>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

/* Local */
#include "Frame.h"
#include "Log.h"

bool pm::RealCamera::s_isInitialized = false;

void pm::RealCamera::TimeLapseCallbackHandler(FRAME_INFO* frameInfo,
        void* RealCamera_pointer)
{
    RealCamera* cam = static_cast<RealCamera*>(RealCamera_pointer);
    cam->HandleTimeLapseEofCallback(frameInfo);
}

pm::RealCamera::RealCamera()
    : m_hCam(-1), // Invalid handle
    m_isImaging(false),
    m_timeLapseFrameCount(0),
    m_timeLapseFuture(),
    m_callbackHandler(nullptr),
    m_callbackContext(nullptr)
{
}

pm::RealCamera::~RealCamera()
{
    StopExp();
    Close();
}

bool pm::RealCamera::Initialize()
{
    if (s_isInitialized)
        return true;

    if (PV_OK != pl_pvcam_init())
    {
        Log::LogE("Failure initializing PVCAM (%s)",
                GetErrorMessage().c_str());
        return false;
    }

    s_isInitialized = true;
    return true;
}

bool pm::RealCamera::Uninitialize()
{
    if (!s_isInitialized)
        return true;

    if (PV_OK != pl_pvcam_uninit())
    {
        Log::LogE("Failure uninitializing PVCAM (%s)",
                GetErrorMessage().c_str());
        return false;
    }

    s_isInitialized = false;
    return true;
}

bool pm::RealCamera::GetCameraCount(int16& count) const
{
    if (PV_OK != pl_cam_get_total(&count))
    {
        Log::LogE("Failure getting camera count (%s)",
                GetErrorMessage().c_str());
        return false;
    }
    return true;
}

bool pm::RealCamera::GetName(int16 index, std::string& name) const
{
    name.clear();

    char camName[CAM_NAME_LEN];
    if (PV_OK != pl_cam_get_name(index, camName))
    {
        Log::LogE("Failed to get name for camera at index %d (%s)", index,
                GetErrorMessage().c_str());
        return false;
    }

    name = camName;
    return true;
}

std::string pm::RealCamera::GetErrorMessage() const
{
    std::string message;

    char errMsg[ERROR_MSG_LEN] = "\0";
    const int16 code = pl_error_code();
    if (PV_OK != pl_error_message(code, errMsg))
    {
        message = std::string("Unable to get error message for error code ")
            + std::to_string(code);
    }
    else
    {
        message = errMsg;
    }

    return message;
}

bool pm::RealCamera::Open(const std::string& name)
{
    if (m_isOpen)
        return true;

    if (PV_OK != pl_cam_open((char*)name.c_str(), &m_hCam, OPEN_EXCLUSIVE))
    {
        Log::LogE("Failure opening camera '%s' (%s)", name.c_str(),
                GetErrorMessage().c_str());
        m_hCam = -1;
        return false;
    }

    m_isOpen = true;
    return true;
}

bool pm::RealCamera::Close()
{
    if (!m_isOpen)
        return true;

    if (PV_OK != pl_cam_close(m_hCam))
    {
        Log::LogD("Failed to close camera, error ignored (%s)",
                GetErrorMessage().c_str());
        // Error ignored, need to uninitialize PVCAM anyway
        //return false;
    }

    DeleteBuffer();

    m_hCam = -1;
    m_isOpen = false;
    return true;
}

bool pm::RealCamera::SetupExp(const SettingsReader& settings)
{
    if (!Camera::SetupExp(settings))
        return false;

    const uns32 acqFrameCount = m_settings.GetAcqFrameCount();
    const uns32 bufferFrameCount = m_settings.GetBufferFrameCount();
    const AcqMode acqMode = m_settings.GetAcqMode();

    const uns32 exposure = m_settings.GetExposure();
    const int16 expMode =
        (int16)m_settings.GetTrigMode() | (int16)m_settings.GetExpOutMode();

    const uns16 rgn_total = (uns16)m_settings.GetRegions().size();
    const rgn_type* rgn_array = m_settings.GetRegions().data();

    uns32 bufferBytes = 0;
    switch (acqMode)
    {
    case AcqMode::SnapSequence:
        if (acqFrameCount > std::numeric_limits<uns16>::max())
        {
            Log::LogE("Too many frames in sequence (%u does not fit in 16 bits)",
                    acqFrameCount);
            return false;
        }
        if (PV_OK != pl_exp_setup_seq(m_hCam, (uns16)acqFrameCount,
                    rgn_total, rgn_array, expMode, exposure, &bufferBytes))
        {
            Log::LogE("Failed to setup sequence acquisition (%s)",
                    GetErrorMessage().c_str());
            return false;
        }

        // Set size of the buffer
        m_frameBytes = bufferBytes / acqFrameCount;
        break;

    case AcqMode::SnapCircBuffer:
    case AcqMode::LiveCircBuffer:
        if (PV_OK != pl_exp_setup_cont(m_hCam, rgn_total, rgn_array, expMode,
                    exposure, &m_frameBytes, CIRC_OVERWRITE))
        {
            Log::LogE("Failed to setup continuous acquisition (%s)",
                    GetErrorMessage().c_str());
            return false;
        }

        // Set size of the buffer
        bufferBytes = m_frameBytes * bufferFrameCount;
        break;

    case AcqMode::SnapTimeLapse:
    case AcqMode::LiveTimeLapse:
        if (PV_OK != pl_exp_setup_seq(m_hCam, 1, rgn_total, rgn_array, expMode,
                    exposure, &m_frameBytes))
        {
            Log::LogE("Failed to setup time-lapse acquisition (%s)",
                    GetErrorMessage().c_str());
            return false;
        }

        // Set size of the buffer
        // This mode uses single frame acq. but we re-use all frames in our buffer
        bufferBytes = m_frameBytes * bufferFrameCount;
        break;
    }

    if (!AllocateBuffer(bufferBytes))
    {
        Log::LogE("Failure allocating buffer with %u bytes", bufferBytes);
        return false;
    }

    m_timeLapseFrameCount = 0;

    return true;
}

bool pm::RealCamera::StartExp(CallbackEx3Fn callbackHandler, void* callbackContext)
{
    if (!callbackHandler || !callbackContext)
        return false;

    m_callbackHandler = callbackHandler;
    m_callbackContext = callbackContext;

    const AcqMode acqMode = m_settings.GetAcqMode();

    if (acqMode == AcqMode::SnapTimeLapse || acqMode == AcqMode::LiveTimeLapse)
    {
        /* Register time lapse callback only at the beginning, that might
           increase performance a bit. */
        if (m_timeLapseFrameCount == 0)
        {
            if (PV_OK != pl_cam_register_callback_ex3(m_hCam, PL_CALLBACK_EOF,
                    (void*)&RealCamera::TimeLapseCallbackHandler, (void*)this))
            {
                Log::LogE("Failed to register EOF callback for time-lapse mode (%s)",
                        GetErrorMessage().c_str());
                return false;
            }
        }
    }
    else
    {
        if (PV_OK != pl_cam_register_callback_ex3(m_hCam, PL_CALLBACK_EOF,
                (void*)m_callbackHandler, m_callbackContext))
        {
            Log::LogE("Failed to register EOF callback (%s)",
                    GetErrorMessage().c_str());
            return false;
        }
    }

    // Tell the camera to start
    bool keepGoing = false;
    switch (acqMode)
    {
    case AcqMode::SnapCircBuffer:
    case AcqMode::LiveCircBuffer:
        keepGoing = (PV_OK == pl_exp_start_cont(m_hCam, m_buffer, m_bufferBytes));
        break;
    case AcqMode::SnapSequence:
        keepGoing = (PV_OK == pl_exp_start_seq(m_hCam, m_buffer));
        break;
    case AcqMode::SnapTimeLapse:
    case AcqMode::LiveTimeLapse:
        // Re-use internal buffer for buffering when sequence has one frame only
        keepGoing = (PV_OK == pl_exp_start_seq(m_hCam,
                (void*)((uns8*)m_buffer + m_frameBytes
                    * (m_timeLapseFrameCount % m_settings.GetBufferFrameCount()))));
        break;
    }
    if (!keepGoing)
    {
        Log::LogE("Failed to start the acquisition (%s)",
                GetErrorMessage().c_str());
        return false;
    }

    m_isImaging = true;

    return true;
}

bool pm::RealCamera::StopExp()
{
    if (m_isImaging)
    {
        // Unconditionally stop the acquisition

        pl_exp_abort(m_hCam, CCS_HALT);
        pl_exp_finish_seq(m_hCam, m_buffer, 0);

        m_isImaging = false;

        // Do not deregister callbacks before pl_exp_abort, abort could freeze then
        pl_cam_deregister_callback(m_hCam, PL_CALLBACK_EOF);

        m_callbackHandler = nullptr;
        m_callbackContext = nullptr;
    }

    return true;
}

bool pm::RealCamera::SetParam(uns32 id, void* param)
{
    return (PV_OK == pl_set_param(m_hCam, id, param));
}

bool pm::RealCamera::GetParam(uns32 id, int16 attr, void* param) const
{
    return (PV_OK == pl_get_param(m_hCam, id, attr, param));
}

bool pm::RealCamera::GetEnumParam(uns32 id, std::vector<EnumItem>& items) const
{
    items.clear();

    uns32 count;
    if (PV_OK != pl_get_param(m_hCam, id, ATTR_COUNT, &count))
        return false;
    for (uns32 n = 0; n < count; n++) {
        uns32 enumStrLen;
        if (PV_OK != pl_enum_str_length(m_hCam, id, n, &enumStrLen))
            return false;

        int32 enumValue;
        char* enumString = new(std::nothrow) char[enumStrLen];
        if (PV_OK != pl_get_enum_param(m_hCam, id, n, &enumValue, enumString,
                    enumStrLen)) {
            return false;
        }
        EnumItem item;
        item.value = enumValue;
        item.desc = enumString;
        items.push_back(item);
        delete [] enumString;
    }

    return true;
}

bool pm::RealCamera::GetLatestFrame(Frame& frame) const
{
    FRAME_INFO frameInfo;
    // Set to an error state before PVCAM tries to reset pointer to valid frame location
    void* data = nullptr;

    // Get the latest frame
    if (PV_OK != pl_exp_get_latest_frame_ex(m_hCam, &data, &frameInfo))
    {
        Log::LogE("Failed to get latest frame from PVCAM (%s)",
                GetErrorMessage().c_str());
        return false;
    }

    if (!data)
    {
        Log::LogE("Invalid latest frame pointer");
        return false;
    }

    // Fix the frame number which is always 1 in time lapse mode
    const AcqMode acqMode = m_settings.GetAcqMode();
    if (acqMode == AcqMode::SnapTimeLapse || acqMode == AcqMode::LiveTimeLapse)
        frameInfo.FrameNr = (int32)m_timeLapseFrameCount;

    // Update local buffer with FRAME_INFO copies
    const size_t offset = ((uns8*)data - (uns8*)m_buffer);
    const size_t index = offset / m_frameBytes;
    if (index * m_frameBytes != offset)
    {
        Log::LogE("Invalid frame data offset");
        return false;
    }
    memcpy(m_fiBuffer.at(index), &frameInfo, sizeof(FRAME_INFO));

    return frame.CopyDataPointer(&frameInfo, data);
}

void pm::RealCamera::HandleTimeLapseEofCallback(FRAME_INFO* frameInfo)
{
    m_timeLapseFrameCount++;

    // Fix the frame number which is always 1 in time lapse mode
    frameInfo->FrameNr = (int32)m_timeLapseFrameCount;

    // Call registered callback
    m_callbackHandler(frameInfo, m_callbackContext);

    // Do not start acquisition for next frame if done
    if (m_timeLapseFrameCount >= m_settings.GetAcqFrameCount()
            && m_settings.GetAcqMode() != AcqMode::LiveTimeLapse)
        return;

    /* m_timeLapseFuture member exists only because of the following fact
       stated in specification:

           "If the std::future obtained from std::async has temporary object
           lifetime (not moved or bound to a variable), the destructor of the
           std::future will block at the end of the full expression until the
           asynchronous operation completes."

       We don't need to wait for any result and we are sure that the future
       either didn't start yet (in case of first frame) or has successfully
       completed (the single frame acq. started again 'cos we are here). */

    m_timeLapseFuture = std::async(std::launch::async, [this]() -> void {
        // Backup callback data, StopExp will clear these members
        CallbackEx3Fn callbackHandler = m_callbackHandler;
        void* callbackContext = m_callbackContext;

        /* No need to stop explicitely, acquisition has already finished and
           we don't want to release allocated buffer. */
        //StopExp();

        if (m_settings.GetTimeLapseDelay() > 0)
            std::this_thread::sleep_for(
                    std::chrono::milliseconds(m_settings.GetTimeLapseDelay()));

        if (!StartExp(callbackHandler, callbackContext))
        {
            /* There is no direct way how to let Acquisition know about error.
               Call the callback handler with null pointer again. */
            m_callbackHandler(nullptr, m_callbackContext);
        }
    });
}
