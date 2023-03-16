#include "Camera.h"

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
#include "Utils.h"

pm::Camera::Camera()
    : m_isOpen(false),
    m_settings(),
    m_frameBytes(0),
    m_buffer(nullptr),
    m_bufferBytes(0),
    m_fiBuffer()
{
}

pm::Camera::~Camera()
{
}

bool pm::Camera::UpdateReadOnlySettings(Settings& settings)
{
    if (!m_isOpen)
        return false;

    // Set port, speed and gain index first
    int32 portIndex = m_settings.GetPortIndex();
    if (!SetParam(PARAM_READOUT_PORT, &portIndex))
    {
        Log::LogE("Failure setting readout port index to %d (%s)", portIndex,
                GetErrorMessage().c_str());
        return false;
    }
    int16 speedIndex = m_settings.GetSpeedIndex();
    if (!SetParam(PARAM_SPDTAB_INDEX, &speedIndex))
    {
        Log::LogE("Failure setting speed index to %d (%s)", speedIndex,
                GetErrorMessage().c_str());
        return false;
    }
    int16 gainIndex = m_settings.GetGainIndex();
    if (!SetParam(PARAM_GAIN_INDEX, &gainIndex))
    {
        Log::LogE("Failure setting gain index to %d (%s)", gainIndex,
                GetErrorMessage().c_str());
        return false;
    }

    uns16 bitDepth;
    if (!GetParam(PARAM_BIT_DEPTH, ATTR_CURRENT, &bitDepth))
    {
        Log::LogE("Failure getting bit depth");
        return false;
    }
    settings.GetReadOnlyWriter().SetBitDepth(bitDepth);

    uns16 width;
    if (!GetParam(PARAM_SER_SIZE, ATTR_CURRENT, &width))
    {
        Log::LogE("Failure getting sensor width");
        return false;
    }
    settings.GetReadOnlyWriter().SetWidth(width);

    uns16 height;
    if (!GetParam(PARAM_PAR_SIZE, ATTR_CURRENT, &height))
    {
        Log::LogE("Failure getting sensor height");
        return false;
    }
    settings.GetReadOnlyWriter().SetHeight(height);

    rs_bool hasCircBuf;
    if (!GetParam(PARAM_CIRC_BUFFER, ATTR_AVAIL, &hasCircBuf))
    {
        Log::LogE("Failure checking circular buffer support");
        return false;
    }
    settings.GetReadOnlyWriter().SetCircBufferCapable(hasCircBuf == TRUE);

    uns16 regionCountMax;
    if (!GetParam(PARAM_ROI_COUNT, ATTR_MAX, &regionCountMax))
    {
        Log::LogE("Failure getting max. ROI count supported");
        return false;
    }
    settings.GetReadOnlyWriter().SetRegionCountMax(regionCountMax);

    rs_bool hasMetadata;
    if (!GetParam(PARAM_METADATA_ENABLED, ATTR_AVAIL, &hasMetadata))
    {
        Log::LogE("Failure checking frame metadata support");
        return false;
    }
    settings.GetReadOnlyWriter().SetMetadataCapable(hasMetadata == TRUE);

    int32 colorMask;
    if (!GetParam(PARAM_COLOR_MODE, ATTR_CURRENT, &colorMask))
    {
        // For monochromatic cameras the parameter is not available
        colorMask = COLOR_NONE;
    }
    settings.GetReadOnlyWriter().SetColorMask(colorMask);

    rs_bool hasCentroids;
    if (!GetParam(PARAM_CENTROIDS_ENABLED, ATTR_AVAIL, &hasCentroids))
    {
        Log::LogE("Failure checking centroids support");
        return false;
    }
    settings.GetReadOnlyWriter().SetCentroidsCapable(hasCentroids == TRUE);

    if (settings.GetCentroidsCapable())
    {
        uns16 centroidCountMax;
        if (!GetParam(PARAM_CENTROIDS_COUNT, ATTR_MAX, &centroidCountMax))
        {
            Log::LogE("Failure getting max. centroid count supported");
            return false;
        }
        settings.GetReadOnlyWriter().SetCentroidCountMax(centroidCountMax);
        if (settings.GetCentroidCount() > centroidCountMax)
        {
            settings.SetCentroidCount(centroidCountMax);
        }

        uns16 centroidRadiusMax;
        if (!GetParam(PARAM_CENTROIDS_RADIUS, ATTR_MAX, &centroidRadiusMax))
        {
            Log::LogE("Failure getting max. centroid count supported");
            return false;
        }
        settings.GetReadOnlyWriter().SetCentroidRadiusMax(centroidRadiusMax);
        if (settings.GetCentroidRadius() > centroidRadiusMax)
        {
            settings.SetCentroidRadius(centroidRadiusMax);
        }
    }

    return true;
}

bool pm::Camera::SetupExp(const SettingsReader& settings)
{
    m_settings = settings;

    if (m_settings.GetRegions().size() > (std::numeric_limits<uns16>::max)()
            || m_settings.GetRegions().empty())
    {
        Log::LogE("Invalid number of regions (%llu)",
                  static_cast<unsigned long long>(m_settings.GetRegions().size()));
        return false;
    }

    const uns32 acqFrameCount = m_settings.GetAcqFrameCount();
    const uns32 bufferFrameCount = m_settings.GetBufferFrameCount();
    const AcqMode acqMode = m_settings.GetAcqMode();

    if (acqMode == AcqMode::SnapSequence && acqFrameCount > bufferFrameCount)
    {
        Log::LogE("When in snap sequence mode, "
                "we cannot acquire more frames than the buffer size (%u)",
                bufferFrameCount);
        return false;
    }

    if ((acqMode == AcqMode::LiveCircBuffer || acqMode == AcqMode::LiveTimeLapse)
            && m_settings.GetStorageType() != StorageType::None
            && m_settings.GetSaveLast() > 0)
    {
        Log::LogE("When in live mode, we cannot save last N frames");
        return false;
    }

    // Set port, speed and gain index first
    int32 portIndex = m_settings.GetPortIndex();
    if (!SetParam(PARAM_READOUT_PORT, &portIndex))
    {
        Log::LogE("Failure setting readout port index to %d (%s)", portIndex,
                GetErrorMessage().c_str());
        return false;
    }
    int16 speedIndex = m_settings.GetSpeedIndex();
    if (!SetParam(PARAM_SPDTAB_INDEX, &speedIndex))
    {
        Log::LogE("Failure setting speed index to %d (%s)", speedIndex,
                GetErrorMessage().c_str());
        return false;
    }
    int16 gainIndex = m_settings.GetGainIndex();
    if (!SetParam(PARAM_GAIN_INDEX, &gainIndex))
    {
        Log::LogE("Failure setting gain index to %d (%s)", gainIndex,
                GetErrorMessage().c_str());
        return false;
    }

    int32 clrMode = m_settings.GetClrMode();
    if (!SetParam(PARAM_CLEAR_MODE, &clrMode))
    {
        Log::LogE("Failure setting clearing mode to %d (%s)", clrMode,
                GetErrorMessage().c_str());
        return false;
    }
    uns16 clrCycles = m_settings.GetClrCycles();
    if (!SetParam(PARAM_CLEAR_CYCLES, &clrCycles))
    {
        Log::LogE("Failure setting clear cycles to %u (%s)", clrCycles,
                GetErrorMessage().c_str());
        return false;
    }

    int32 expTimeRes = m_settings.GetExposureResolution();
    const char* resName = "<UNKNOWN>";
    switch (expTimeRes)
    {
    case EXP_RES_ONE_MICROSEC:
        resName = "microseconds";
        break;
    case EXP_RES_ONE_MILLISEC:
        resName = "milliseconds";
        break;
    case EXP_RES_ONE_SEC:
        resName = "seconds";
        break;
    }
    if (!SetParam(PARAM_EXP_RES, &expTimeRes))
    {
        Log::LogE("Failure setting exposure resolution to %s (%d) (%s)",
                resName, expTimeRes, GetErrorMessage().c_str());
        return false;
    }

    // TODO: Handle PMODE in Settings, here we enforce default value only
    int32 pMode;
    if (!GetParam(PARAM_PMODE, ATTR_DEFAULT, &pMode))
    {
        Log::LogE("Failure getting default clocking mode");
        return false;
    }
    if (!SetParam(PARAM_PMODE, &pMode))
    {
        Log::LogE("Failure setting default clocking mode %d", pMode);
        return false;
    }

    rs_bool trigTabSignalAvail;
    rs_bool lastMuxedSignalAvail;
    if (GetParam(PARAM_TRIGTAB_SIGNAL, ATTR_AVAIL, &trigTabSignalAvail)
            && GetParam(PARAM_LAST_MUXED_SIGNAL, ATTR_AVAIL, &lastMuxedSignalAvail)
            && trigTabSignalAvail == TRUE && lastMuxedSignalAvail == TRUE)
    {
        int32 trigTabSignal = m_settings.GetTrigTabSignal();
        if (!SetParam(PARAM_TRIGTAB_SIGNAL, &trigTabSignal))
        {
            Log::LogE("Failure setting triggering table signal to %d (%s)",
                    trigTabSignal, GetErrorMessage().c_str());
            return false;
        }
        uns8 lastMuxedSignal = m_settings.GetLastMuxedSignal();
        if (!SetParam(PARAM_LAST_MUXED_SIGNAL, &lastMuxedSignal))
        {
            Log::LogE("Failure setting last multiplexed signal to %u (%s)",
                    lastMuxedSignal, GetErrorMessage().c_str());
            return false;
        }
    }

    if (m_settings.GetMetadataCapable())
    {
        rs_bool enabled = m_settings.GetMetadataEnabled();
        if (!SetParam(PARAM_METADATA_ENABLED, &enabled))
        {
            if (enabled)
                Log::LogE("Failure enabling frame metadata (%s)",
                        GetErrorMessage().c_str());
            else
                Log::LogE("Failure disabling frame metadata (%s)",
                        GetErrorMessage().c_str());
            return false;
        }
    }
    else
    {
        if (m_settings.GetMetadataEnabled())
        {
            Log::LogE("Unable to enable frame metadata, camera does not support it");
            return false;
        }
    }

    if (m_settings.GetCentroidsCapable())
    {
        rs_bool enabled = m_settings.GetCentroidsEnabled();
        if (!SetParam(PARAM_CENTROIDS_ENABLED, &enabled))
        {
            if (enabled)
                Log::LogE("Failure enabling centroids (%s)",
                        GetErrorMessage().c_str());
            else
                Log::LogE("Failure disabling centroids (%s)",
                        GetErrorMessage().c_str());
            return false;
        }
        if (enabled)
        {
            uns16 count = m_settings.GetCentroidCount();
            if (!SetParam(PARAM_CENTROIDS_COUNT, &count))
            {
                Log::LogE("Failure setting centroid count to %u (%s)", count,
                        GetErrorMessage().c_str());
                return false;
            }
            uns16 radius = m_settings.GetCentroidRadius();
            if (!SetParam(PARAM_CENTROIDS_RADIUS, &radius))
            {
                Log::LogE("Failure setting centroid radius (%s)",
                        GetErrorMessage().c_str());
                return false;
            }
        }
    }
    else
    {
        if (m_settings.GetCentroidsEnabled())
        {
            Log::LogE("Unable to enable centroids, camera does not support it");
            return false;
        }
    }

    // Setup the acquisition and call AllocateBuffer in derived class

    return true;
}

bool pm::Camera::GetFrameAt(size_t index, Frame& frame) const
{
    if (m_bufferBytes == 0 || !m_buffer)
        return false;

    if ((index + 1) * m_frameBytes > m_bufferBytes
            || index >= m_fiBuffer.size())
    {
        Log::LogE("Frame index out of buffer boundaries");
        return false;
    }

    void* data = (uns8*)m_buffer + index * m_frameBytes;
    const FRAME_INFO* frameInfo = m_fiBuffer.at(index);

    return frame.CopyDataPointer(frameInfo, data);
}

bool pm::Camera::GetFrameIndex(const Frame& frame, size_t& index) const
{
    if (!frame.GetFrameInfo())
    {
        Log::LogE("Invalid frame info");
        return false;
    }

    const int32 frameNr = frame.GetFrameInfo()->FrameNr;
    for (size_t n = 0; n < m_fiBuffer.size(); n++)
    {
        if (m_fiBuffer.at(n)->FrameNr == frameNr)
        {
            index = n;
            return true;
        }
    }

    Log::LogE("Frame index for frameNr=%d not found", frameNr);
    return false;
}

bool pm::Camera::AllocateBuffer(uns32 bufferBytes)
{
    if (m_bufferBytes == bufferBytes && m_buffer)
        return true;

    DeleteBuffer();

    if (bufferBytes == 0)
        return false;

    const uns32 bufferFrameCount = m_settings.GetBufferFrameCount();
    for (uns32 n = 0; n < bufferFrameCount; n++)
    {
        FRAME_INFO* fi = nullptr;
        if (PV_OK != pl_create_frame_info_struct(&fi) || !fi)
            return false;
        m_fiBuffer.push_back(fi);
    }

    /*
        HACK: THIS IS VERY DIRTY HACK!!!
        Because of heap corruption that occurs at least with PCIe cameras
        and ROI having position and size with odd numbers, we allocate here
        additional 8 bytes.
        Example rgn_type could be [123,881,1,135,491,1].
        During long investigation I've seen 2, 4 or 6 bytes behind m_buffer
        are always filled with value 0x1c comming probably from PCIe
        driver.
    */
    m_buffer = (void*)new(std::nothrow) uns8[bufferBytes + 8];
    if (!m_buffer)
        return false;

    m_bufferBytes = bufferBytes;

    return true;
}

void pm::Camera::DeleteBuffer()
{
    delete [] (uns8*)m_buffer;
    m_buffer = nullptr;

    m_bufferBytes = 0;

    for (FRAME_INFO* fi : m_fiBuffer)
    {
        // Ignore errors
        if (fi)
            pl_release_frame_info_struct(fi);
    }
    m_fiBuffer.clear();
}
