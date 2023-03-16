#include "Acquisition.h"

/* System */
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

/* Local */
#include "Camera.h"
#include "Frame.h"
#include "Log.h"
#include "PrdFileSave.h"
#include "TiffFileSave.h"
#include "Utils.h"

void pm::Acquisition::EofCallback(FRAME_INFO* frameInfo, void* Acquisition_pointer)
{
    Acquisition* acq = static_cast<Acquisition*>(Acquisition_pointer);
    if (!frameInfo || !acq->HandleEofCallback(frameInfo))
        acq->RequestAbort();
}

pm::Acquisition::Acquisition(std::shared_ptr<Camera> camera)
    : m_camera(camera),
    m_fpsLimiter(nullptr),
    m_maxFramesPerStack(0),
    m_uncaughtFrames(),
    m_unsavedFrames(),
    m_acqThread(nullptr),
    m_diskThread(nullptr),
    m_updateThread(nullptr),
    m_abortFlag(false),
    m_acqTime(0.0),
    m_diskTime(0.0),
    m_lastFrameNumber(0),
    m_outOfOrderFrameCount(0),
    m_updateThreadMutex(),
    m_updateThreadCond(),
    m_updateThreadStopFlag(false),
    m_toBeProcessedFrames(),
    m_toBeProcessedFramesMutex(),
    m_toBeProcessedFramesCond(),
    m_toBeProcessedFramesMax(1),
    m_toBeProcessedFramesMaxPeak(0),
    m_toBeProcessedFramesLost(0),
    m_toBeProcessedFramesProcessed(0),
    m_toBeSavedFrames(),
    m_toBeSavedFramesMutex(),
    m_toBeSavedFramesCond(),
    m_toBeSavedFramesMax(1),
    m_toBeSavedFramesMaxPeak(0),
    m_toBeSavedFramesLost(0),
    m_toBeSavedFramesSaved(0),
    m_unusedFrames(),
    m_unusedFramesMutex()
{
}

pm::Acquisition::~Acquisition()
{
    RequestAbort();
    WaitForStop();

    // No need to lock queues below, they are private and all threads are stopped

    // Free unused frames
    while (!m_toBeProcessedFrames.empty())
    {
        m_toBeProcessedFrames.front().reset();
        m_toBeProcessedFrames.pop();
    }

    // Free queued but not processed frames
    while (!m_toBeSavedFrames.empty())
    {
        m_toBeSavedFrames.front().reset();
        m_toBeSavedFrames.pop();
    }

    // Free unused frames
    while (!m_unusedFrames.empty())
    {
        m_unusedFrames.front().reset();
        m_unusedFrames.pop();
    }
}

bool pm::Acquisition::Start(std::shared_ptr<FpsLimiter> fpsLimiter)
{
    if (!m_camera)
        return false;

    if (IsRunning())
        return true;

    /* The option below is used for testing purposes, but also for
        demonstration in terms of what places in the code would need to be
        altered to NOT save frames to disk */
    if (!ConfigureStorage())
        return false;

    if (!PreallocateUnusedFrames())
        return false;

    m_fpsLimiter = fpsLimiter;

    m_abortFlag = false;
    m_updateThreadStopFlag = false;
    // m_acqTime is checked during threads startup so reset it
    m_acqTime = 0.0;

    /* Start all threads but acquisition first to reduce the overall system
       load after starting the acquisition */
    m_diskThread =
        new(std::nothrow) std::thread(&Acquisition::DiskThreadLoop, this);
    if (m_diskThread)
    {
        m_updateThread =
            new(std::nothrow) std::thread(&Acquisition::UpdateThreadLoop, this);
        if (m_updateThread)
        {
            m_acqThread =
                new(std::nothrow) std::thread(&Acquisition::AcqThreadLoop, this);
        }

        if (!m_updateThread || !m_acqThread)
        {
            RequestAbort();
            WaitForStop(); // Returns true - aborted
        }
    }

    return IsRunning();
}

bool pm::Acquisition::IsRunning() const
{
    return (m_acqThread || m_diskThread || m_updateThread);
}

void pm::Acquisition::RequestAbort()
{
    m_abortFlag = true;

    // Wake up all possible waiters
    m_toBeProcessedFramesCond.notify_all();
    m_toBeSavedFramesCond.notify_all();
    m_updateThreadCond.notify_all();
}

bool pm::Acquisition::WaitForStop(bool printStats)
{
    if (m_acqThread)
    {
        if (m_acqThread->joinable())
            m_acqThread->join();
        delete m_acqThread;
        m_acqThread = nullptr;
    }
    if (m_diskThread)
    {
        if (m_diskThread->joinable())
            m_diskThread->join();
        delete m_diskThread;
        m_diskThread = nullptr;
    }
    if (m_updateThread)
    {
        if (m_updateThread->joinable())
            m_updateThread->join();
        delete m_updateThread;
        m_updateThread = nullptr;
    }

    if (printStats)
    {
        PrintAcqThreadStats();
        PrintDiskThreadStats();
    }

    return m_abortFlag;
}

std::unique_ptr<pm::Frame> pm::Acquisition::AllocateNewFrame()
{
    /* We use references when in sequence mode because the frame data isn't
       going anywhere so there's no reason to copy it out of PVCAM's buffer */

    return std::unique_ptr<Frame>(new(std::nothrow) Frame(m_camera->GetFrameBytes(),
            m_camera->GetSettings().GetAcqMode() != AcqMode::SnapSequence));
    /* TODO: Later, with C++14, we should rewrite it using make_unique.
       On Windows MSVS 2013 compiles it already but on Linux is required
       GCC 4.9 and above with CXXFLAGS having -std=c++1y which is not available
       by default in any major distribution yet. */
    //return std::make_unique<Frame>(m_camera->GetFrameBytes(),
    //        m_camera->GetSettings().GetAcqMode() != AcqMode::SnapSequence));
}

void pm::Acquisition::UnuseFrame(std::unique_ptr<Frame> frame)
{
    std::unique_lock<std::mutex> lock(m_unusedFramesMutex);
    m_unusedFrames.push(std::move(frame));
}

bool pm::Acquisition::HandleEofCallback(FRAME_INFO* frameInfo)
{
    if (m_abortFlag)
        return false; // Return value doesn't matter, abort is already in progress

    std::unique_ptr<Frame> frame = nullptr;
    size_t capturedFramesSize;
    {
        std::unique_lock<std::mutex> lock(m_toBeProcessedFramesMutex);
        capturedFramesSize = m_toBeProcessedFrames.size();
    }
    {
        std::unique_lock<std::mutex> lock(m_unusedFramesMutex);
        // Get free frame buffer, either create new or re-use old one
        if (capturedFramesSize < m_toBeProcessedFramesMax)
        {
            if (!m_unusedFrames.empty())
            {
                frame = std::move(m_unusedFrames.front());
                m_unusedFrames.pop();
            }
            else
            {
                frame = std::move(AllocateNewFrame());
            }
        }
    }
    if (!frame)
        // No room for new frame, dropping it and keep going
        return true;

    if (!m_camera->GetLatestFrame(*frame))
    {
        // Abort, could happen e.g. if frame number is 0
        UnuseFrame(std::move(frame));
        return false;
    }

    if (0 != memcmp(frameInfo, frame->GetFrameInfo(), sizeof(*frameInfo)))
    {
        Log::LogE("Frame info does not match: frame number from callback is %d"
                " vs. %d latest frame", frameInfo->FrameNr, m_lastFrameNumber);

        // Abort, structures have to be same if GetLatestFrame is called from callback
        UnuseFrame(std::move(frame));
        return false;
    }

    // Put frame to queue for processing
    {
        std::unique_lock<std::mutex> lock(m_toBeProcessedFramesMutex);

        m_toBeProcessedFrames.push(std::move(frame));

        if (m_toBeProcessedFramesMaxPeak < m_toBeProcessedFrames.size())
            m_toBeProcessedFramesMaxPeak = m_toBeProcessedFrames.size();
    }
    // Notify all waiters about new captured frame
    m_toBeProcessedFramesCond.notify_all();

    return true;
}

bool pm::Acquisition::HandleNewFrame(std::unique_ptr<Frame> frame)
{
    // Do deep copy
    if (!frame->CopyData())
        return false;

    if (m_fpsLimiter)
    {
        // Pass frame copy to FPS limiter for later processing
        m_fpsLimiter->InputNewFrame(frame->Clone());
    }

    const FRAME_INFO* frameInfo = frame->GetFrameInfo();

    if (frameInfo->FrameNr <= m_lastFrameNumber)
    {
        m_outOfOrderFrameCount++;

        Log::LogE("Frame number out of order: %d, last frame number was %d, ignoring",
                frameInfo->FrameNr, m_lastFrameNumber);

        m_toBeProcessedFramesLost++;

        // Drop frame for invalid frame number
        UnuseFrame(std::move(frame));

        // Number out of order, cannot add it to m_unsavedFrames stats

        return true;
    }

    // Check to make sure we didn't skip a frame
    const uint32_t lostFrameCount = frameInfo->FrameNr - m_lastFrameNumber - 1;
    if (lostFrameCount > 0)
    {
        m_toBeProcessedFramesLost += lostFrameCount;

        // Log all the frame numbers we missed
        for (int32 frameNumber = m_lastFrameNumber + 1;
                frameNumber < frameInfo->FrameNr; frameNumber++)
        {
            // TODO: Handle return value, abort on failure?
            m_uncaughtFrames.AddItem(frameNumber);
        }
    }
    m_lastFrameNumber = frameInfo->FrameNr;

    m_toBeProcessedFramesProcessed++;

    {
        std::unique_lock<std::mutex> saveLock(m_toBeSavedFramesMutex);

        if (m_toBeSavedFrames.size() < m_toBeSavedFramesMax)
        {
            m_toBeSavedFrames.push(std::move(frame));

            if (m_toBeSavedFramesMaxPeak < m_toBeSavedFrames.size())
                m_toBeSavedFramesMaxPeak = m_toBeSavedFrames.size();
        }
        else
        {
            saveLock.unlock();

            m_toBeSavedFramesLost++;

            // Drop the frame, not enough RAM to queue it for saving
            UnuseFrame(std::move(frame));

            // Log the dropped frame
            m_unsavedFrames.AddItem(
                    m_toBeProcessedFramesProcessed + m_toBeProcessedFramesLost);
        }
    }
    // Notify all waiters about new queued frame
    m_toBeSavedFramesCond.notify_all();

    return true;
}

void pm::Acquisition::UpdateToBeSavedFramesMax()
{
    static const size_t totalRamMB = GetTotalRamMB();
    const size_t availRamMB = GetAvailRamMB();
    /* We allow allocation of memory up to bigger value from these:
       - 90% of total RAM
       - whole available RAM reduced by 1GB */
    const size_t dontTouchRamMB = std::min<size_t>(totalRamMB * 10 / 100, 1024);
    const size_t maxFreeRamMB =
        (availRamMB >= dontTouchRamMB) ? availRamMB - dontTouchRamMB : 0;
    // Left shift by 20 bits "converts" megabytes to bytes
    const size_t maxFreeRamBytes = maxFreeRamMB << 20;

    const uint32_t frameBytes = m_camera->GetFrameBytes();
    const size_t maxNewFrameCount =
        (frameBytes == 0) ? 0 : maxFreeRamBytes / frameBytes;

    {
        std::unique_lock<std::mutex> lock(m_toBeSavedFramesMutex);
        m_toBeSavedFramesMax = m_toBeSavedFrames.size() + maxNewFrameCount;
    }
}

bool pm::Acquisition::PreallocateUnusedFrames()
{
    // Limit the queue with captured frames to half of the circular buffer size
    m_toBeProcessedFramesMax =
        (m_camera->GetSettings().GetBufferFrameCount() / 2) + 1;

    UpdateToBeSavedFramesMax();

    const uint32_t frameCount = m_camera->GetSettings().GetAcqFrameCount();
    const uint32_t frameBytes = m_camera->GetFrameBytes();
    const uint32_t frameCountIn100MB =
        (frameBytes == 0) ? 0 : ((100 << 20) / frameBytes);
    const size_t recommendedFrameCount = std::min<size_t>(
            10 + std::min(frameCount, frameCountIn100MB),
            m_toBeSavedFramesMax);

    // Moved unprocessed frames to unused frames queue
    while (!m_toBeProcessedFrames.empty())
    {
        std::unique_ptr<Frame> frame = std::move(m_toBeProcessedFrames.front());
        m_toBeProcessedFrames.pop();
        m_unusedFrames.push(std::move(frame));
    }
    // Moved unsaved frames to unused frames queue
    while (!m_toBeSavedFrames.empty())
    {
        std::unique_ptr<Frame> frame = std::move(m_toBeSavedFrames.front());
        m_toBeSavedFrames.pop();
        m_unusedFrames.push(std::move(frame));
    }

    if (!m_unusedFrames.empty()
            && m_unusedFrames.front()->GetFrameBytes() != frameBytes)
    {
        // Release all frames, frame size has changed
        while (!m_unusedFrames.empty())
        {
            m_unusedFrames.front().reset();
            m_unusedFrames.pop();
        }
    }
    else
    {
        // Release surplus frames
        while (m_unusedFrames.size() > recommendedFrameCount)
        {
            m_unusedFrames.front().reset();
            m_unusedFrames.pop();
        }
    }

    // Allocate some ready-to-use frames
    while (m_unusedFrames.size() < recommendedFrameCount)
    {
        std::unique_ptr<Frame> frame = std::move(AllocateNewFrame());
        if (!frame)
            return false;
        m_unusedFrames.push(std::move(frame));
    }

    return true;
}

bool pm::Acquisition::ConfigureStorage()
{
    const size_t maxStackSize = m_camera->GetSettings().GetMaxStackSize();

    const rgn_type rgn = SettingsReader::GetImpliedRegion(
            m_camera->GetSettings().GetRegions());
    PrdHeader prdHeader;
    ClearPrdHeaderStructure(prdHeader);
    prdHeader.version = PRD_VERSION_0_4;
    prdHeader.bitDepth = m_camera->GetSettings().GetBitDepth();
    memcpy(&prdHeader.region, &rgn, sizeof(PrdRegion));
    prdHeader.sizeOfPrdMetaDataStruct = sizeof(PrdMetaData);
    switch (m_camera->GetSettings().GetExposureResolution())
    {
    case EXP_RES_ONE_MICROSEC:
        prdHeader.exposureResolution = PRD_EXP_RES_US;
        break;
    case EXP_RES_ONE_MILLISEC:
    default: // Just in case, should never happen
        prdHeader.exposureResolution = PRD_EXP_RES_MS;
        break;
    case EXP_RES_ONE_SEC:
        prdHeader.exposureResolution = PRD_EXP_RES_S;
        break;
    }
    prdHeader.colorMask = (uint8_t)m_camera->GetSettings().GetColorMask(); // Since v0.3
    prdHeader.flags = (m_camera->GetSettings().GetMetadataEnabled())
        ? PRD_FLAG_HAS_METADATA : 0x00; // Since v0.3
    prdHeader.frameSize = m_camera->GetFrameBytes(); // Since v0.3

    prdHeader.frameCount = 1;
    const size_t prdSingleBytes = GetPrdFileSizeInBytes(prdHeader);
    Log::LogI("Size of PRD file with single frame: %llu bytes",
            (unsigned long long)prdSingleBytes);

    m_maxFramesPerStack = GetFrameCountThatFitsIn(prdHeader, maxStackSize);

    if (maxStackSize > 0)
    {
        prdHeader.frameCount = m_maxFramesPerStack;
        const size_t prdStackBytes = GetPrdFileSizeInBytes(prdHeader);
        Log::LogI("Max. size of PRD file with up to %u stacked frames: %llu bytes",
                m_maxFramesPerStack, (unsigned long long)prdStackBytes);

        if (m_maxFramesPerStack < 2)
        {
            Log::LogE("Stack size is too small");
            return false;
        }
    }

    UpdateToBeSavedFramesMax();

    return true;
}

void pm::Acquisition::AcqThreadLoop()
{
    Timer acqTimer;
    m_acqTime = 0.0;

    m_toBeProcessedFramesProcessed = 0;
    m_toBeProcessedFramesLost = 0;
    m_lastFrameNumber = 0;
    m_outOfOrderFrameCount = 0;
    m_uncaughtFrames.Clear();

    const size_t frameCount = m_camera->GetSettings().GetAcqFrameCount();
    const AcqMode acqMode = m_camera->GetSettings().GetAcqMode();

    if (!m_camera->StartExp(&Acquisition::EofCallback, this))
    {
        RequestAbort();
    }
    else
    {
        acqTimer.Reset(); // Start up might take some time, ignore it

        Log::LogI("Acquisition has started successfully");

        while ((m_toBeProcessedFramesProcessed + m_toBeProcessedFramesLost < frameCount
                    || acqMode == AcqMode::LiveCircBuffer
                    || acqMode == AcqMode::LiveTimeLapse)
                && !m_abortFlag)
        {
            std::unique_ptr<Frame> frame = nullptr;
            {
                std::unique_lock<std::mutex> lock(m_toBeProcessedFramesMutex);

                if (m_toBeProcessedFrames.empty())
                {
                    m_toBeProcessedFramesCond.wait(lock, [this]() {
                        return (m_abortFlag || !m_toBeProcessedFrames.empty());
                    });
                }
                if (m_abortFlag)
                    break;

                frame = std::move(m_toBeProcessedFrames.front());
                m_toBeProcessedFrames.pop();
            }
            // frame is always valid here
            if (!HandleNewFrame(std::move(frame)))
                RequestAbort();
        }

        m_acqTime = acqTimer.Seconds();

        m_camera->StopExp();
    }

    if (m_acqTime > 0.0)
    {
        std::ostringstream ss;
        ss << (m_toBeProcessedFramesProcessed + m_toBeProcessedFramesLost)
            << " frames were acquired from the camera and "
            << m_toBeProcessedFramesProcessed
            << " of them were queued for saving in " << m_acqTime << " seconds"
            << std::endl;
        Log::LogI(ss.str());
    }
}

void pm::Acquisition::DiskThreadLoop()
{
    Timer diskTimer;
    m_diskTime = 0.0;

    m_toBeSavedFramesSaved = 0;
    m_toBeSavedFramesLost = 0;

    const StorageType storageType = m_camera->GetSettings().GetStorageType();
    const size_t maxStackSize = m_camera->GetSettings().GetMaxStackSize();

    if (maxStackSize > 0)
        DiskThreadLoop_Stack();
    else
        DiskThreadLoop_Single();

    m_diskTime = diskTimer.Seconds();

    // Stop updateThread thread
    m_updateThreadStopFlag = true;
    m_updateThreadCond.notify_one();
    m_updateThread->join();

    if (m_diskTime > 0.0)
    {
        std::ostringstream ss;
        ss << m_toBeProcessedFramesProcessed << " queued frames were processed and ";
        switch (storageType)
        {
        case StorageType::Prd:
            ss << m_toBeSavedFramesSaved << " of them were saved to PRD file(s)";
            break;
        case StorageType::Tiff:
            ss << m_toBeSavedFramesSaved << " of them were saved to TIFF file(s)";
            break;
        case StorageType::None:
            ss << " none of them were saved";
            break;
        // No default section, compiler will complain when new format added
        }
        ss << " in " << m_diskTime << " seconds";
        Log::LogI(ss.str());
    }
}

void pm::Acquisition::DiskThreadLoop_Single()
{
    const size_t frameCount = m_camera->GetSettings().GetAcqFrameCount();
    const StorageType storageType = m_camera->GetSettings().GetStorageType();
    const std::string saveDir = m_camera->GetSettings().GetSaveDir();
    const size_t saveFirst =
        std::min(frameCount, m_camera->GetSettings().GetSaveFirst());
    const size_t saveLast =
        std::min(frameCount, m_camera->GetSettings().GetSaveLast());
    const AcqMode acqMode = m_camera->GetSettings().GetAcqMode();

    const rgn_type rgn = SettingsReader::GetImpliedRegion(
            m_camera->GetSettings().GetRegions());
    PrdHeader prdHeader;
    ClearPrdHeaderStructure(prdHeader);
    prdHeader.version = PRD_VERSION_0_4;
    prdHeader.bitDepth = m_camera->GetSettings().GetBitDepth();
    memcpy(&prdHeader.region, &rgn, sizeof(PrdRegion));
    prdHeader.sizeOfPrdMetaDataStruct = sizeof(PrdMetaData);
    switch (m_camera->GetSettings().GetExposureResolution())
    {
    case EXP_RES_ONE_MICROSEC:
        prdHeader.exposureResolution = PRD_EXP_RES_US;
        break;
    case EXP_RES_ONE_MILLISEC:
    default: // Just in case, should never happen
        prdHeader.exposureResolution = PRD_EXP_RES_MS;
        break;
    case EXP_RES_ONE_SEC:
        prdHeader.exposureResolution = PRD_EXP_RES_S;
        break;
    }
    prdHeader.frameCount = 1;
    prdHeader.colorMask = (uint8_t)m_camera->GetSettings().GetColorMask(); // Since v0.3
    prdHeader.flags = (m_camera->GetSettings().GetMetadataEnabled())
        ? PRD_FLAG_HAS_METADATA : 0x00; // Since v0.3
    prdHeader.frameSize = m_camera->GetFrameBytes(); // Since v0.3

    // Absolute frame index in saving sequence
    size_t frameIndex = 0;

    while ((frameIndex < frameCount
                || acqMode == AcqMode::LiveCircBuffer
                || acqMode == AcqMode::LiveTimeLapse)
            && !m_abortFlag)
    {
        std::unique_ptr<Frame> frame = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_toBeSavedFramesMutex);

            if (m_toBeSavedFrames.empty())
            {
                // There are no queued frames and acquisition has finished, stop this thread
                if (m_acqTime > 0.0)
                    break;

                m_toBeSavedFramesCond.wait(lock, [this]() {
                    return (m_abortFlag || !m_toBeSavedFrames.empty());
                });
            }
            if (m_abortFlag)
                break;

            frame = std::move(m_toBeSavedFrames.front());
            m_toBeSavedFrames.pop();
        }

        bool keepGoing = true;

        const bool doSaveFirst = saveFirst > 0 && frameIndex < saveFirst;
        const bool doSaveLast = saveLast > 0 && frameIndex >= frameCount - saveLast;
        const bool doSaveAll = (saveFirst == 0 && saveLast == 0);
        const bool doSave = doSaveFirst || doSaveLast || doSaveAll;

        if (storageType != StorageType::None && doSave)
        {
            // Used frame number instead of frameIndex
            std::string fileName;
            fileName = ((saveDir.empty()) ? "." : saveDir) + "/";
            fileName += "ss_single_"
                + std::to_string(frame->GetFrameInfo()->FrameNr);
            FileSave* file = nullptr;

            switch (storageType)
            {
            case StorageType::Prd:
                fileName += ".prd";
                file = new(std::nothrow) PrdFileSave(fileName, prdHeader);
                break;
            case StorageType::Tiff:
                fileName += ".tiff";
                file = new(std::nothrow) TiffFileSave(fileName, prdHeader);
                break;
            case StorageType::None:
                break;
            // No default section, compiler will complain when new format added
            }

            if (!file || !file->Open())
            {
                Log::LogE("Error in writing data at %s", fileName.c_str());
                keepGoing = false;
            }
            else
            {
                const FRAME_INFO* fi = frame->GetFrameInfo();
                const uint64_t bof = (uint64_t)fi->TimeStampBOF * 100;
                const uint64_t eof = (uint64_t)fi->TimeStamp * 100;

                PrdMetaData metaData;
                memset(&metaData, 0, sizeof(PrdMetaData));
                // PRD v0.1 data
                metaData.frameNumber = (uint32_t)fi->FrameNr;
                metaData.readoutTime = (uint32_t)fi->ReadoutTime * 100;
                metaData.exposureTime =
                    (uint32_t)m_camera->GetSettings().GetExposure();
                // PRD v0.2 data
                metaData.bofTime = (uint32_t)(bof & 0xFFFFFFFF);
                metaData.eofTime = (uint32_t)(eof & 0xFFFFFFFF);
                // PRD v0.3 data
                metaData.roiCount =
                    (uint16_t)m_camera->GetSettings().GetRegions().size();
                // PRD v0.4 data
                metaData.bofTimeHigh = (uint32_t)((bof >> 32) & 0xFFFFFFFF);
                metaData.eofTimeHigh = (uint32_t)((eof >> 32) & 0xFFFFFFFF);

                if (!file->WriteFrame(&metaData, (uint16_t*)frame->GetData()))
                {
                    Log::LogE("Error in writing RAW data at %s",
                            fileName.c_str());
                    keepGoing = false;
                }
                else
                {
                    m_toBeSavedFramesSaved++;
                }

                file->Close();
                delete file;
            }
        }

        if (!keepGoing)
            RequestAbort();

        UnuseFrame(std::move(frame));

        frameIndex++;
    }
}

void pm::Acquisition::DiskThreadLoop_Stack()
{
    /* Logic is very similar to code in DiskThreadLoop_Single method but
       the index arithmetics is much more complicated. */

    const size_t frameCount = m_camera->GetSettings().GetAcqFrameCount();
    const StorageType storageType = m_camera->GetSettings().GetStorageType();
    const std::string saveDir = m_camera->GetSettings().GetSaveDir();
    const size_t saveFirst =
        std::min(frameCount, m_camera->GetSettings().GetSaveFirst());
    const size_t saveLast =
        std::min(frameCount, m_camera->GetSettings().GetSaveLast());
    const size_t maxStackSize = m_camera->GetSettings().GetMaxStackSize();
    const AcqMode acqMode = m_camera->GetSettings().GetAcqMode();

    const rgn_type rgn = SettingsReader::GetImpliedRegion(
            m_camera->GetSettings().GetRegions());
    PrdHeader prdHeader;
    ClearPrdHeaderStructure(prdHeader);
    prdHeader.version = PRD_VERSION_0_4;
    prdHeader.bitDepth = m_camera->GetSettings().GetBitDepth();
    memcpy(&prdHeader.region, &rgn, sizeof(PrdRegion));
    prdHeader.sizeOfPrdMetaDataStruct = sizeof(PrdMetaData);
    switch (m_camera->GetSettings().GetExposureResolution())
    {
    case EXP_RES_ONE_MICROSEC:
        prdHeader.exposureResolution = PRD_EXP_RES_US;
        break;
    case EXP_RES_ONE_MILLISEC:
    default: // Just in case, should never happen
        prdHeader.exposureResolution = PRD_EXP_RES_MS;
        break;
    case EXP_RES_ONE_SEC:
        prdHeader.exposureResolution = PRD_EXP_RES_S;
        break;
    }
    //prdHeader.frameCount calculated later
    prdHeader.colorMask = (uint8_t)m_camera->GetSettings().GetColorMask(); // Since v0.3
    prdHeader.flags = (m_camera->GetSettings().GetMetadataEnabled())
        ? PRD_FLAG_HAS_METADATA : 0x00; // Since v0.3
    prdHeader.frameSize = m_camera->GetFrameBytes(); // Since v0.3

    const uint32_t maxFramesPerStack = GetFrameCountThatFitsIn(prdHeader, maxStackSize);

    std::string fileName;
    FileSave* file = nullptr;

    // Absolute frame index in saving sequence
    size_t frameIndex = 0;

    while ((frameIndex < frameCount
                || acqMode == AcqMode::LiveCircBuffer
                || acqMode == AcqMode::LiveTimeLapse)
            && !m_abortFlag)
    {
        std::unique_ptr<Frame> frame = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_toBeSavedFramesMutex);

            if (m_toBeSavedFrames.empty())
            {
                // There are no queued frames and acquisition has finished, stop this thread
                if (m_acqTime > 0.0)
                    break;

                m_toBeSavedFramesCond.wait(lock, [this]() {
                    return (m_abortFlag || !m_toBeSavedFrames.empty());
                });
            }
            if (m_abortFlag)
                break;

            frame = std::move(m_toBeSavedFrames.front());
            m_toBeSavedFrames.pop();
        }

        bool keepGoing = true;

        const bool doSaveFirst = saveFirst > 0 && frameIndex < saveFirst;
        const bool doSaveLast = saveLast > 0 && frameIndex >= frameCount - saveLast;
        const bool doSaveAll = (saveFirst == 0 && saveLast == 0)
            || saveFirst >= frameCount - saveLast;
        const bool doSave = doSaveFirst || doSaveLast || doSaveAll;

        if (storageType != StorageType::None && doSave)
        {
            /* Index for output file, relative either to sequence beginning
                or to first frame for --save-last option */
            size_t stackIndex;
            // Relative frame index in file, first in file is 0
            size_t frameIndexInStack;
            if (doSaveFirst || doSaveAll)
            {
                stackIndex = frameIndex / maxFramesPerStack;
                frameIndexInStack = frameIndex % maxFramesPerStack;
            }
            else // doSaveLast
            {
                stackIndex =
                    (frameIndex - (frameCount - saveLast)) / maxFramesPerStack;
                frameIndexInStack =
                    (frameIndex - (frameCount - saveLast)) % maxFramesPerStack;
            }

            // First frame in new stack, close previous file and open new one
            if (frameIndexInStack == 0)
            {
                // Close previous file if some open
                if (file)
                {
                    file->Close();
                    delete file;
                    file = nullptr;
                }

                // Calculate number of frames in this file and set name
                fileName = ((saveDir.empty()) ? "." : saveDir) + "/";
                if (doSaveAll)
                {
                    if (stackIndex < (frameCount - 1) / maxFramesPerStack)
                        prdHeader.frameCount = maxFramesPerStack;
                    else
                        prdHeader.frameCount = ((frameCount - 1) % maxFramesPerStack) + 1;

                    fileName += "ss_stack_";
                }
                else if (doSaveFirst)
                {
                    if (stackIndex < (saveFirst - 1) / maxFramesPerStack)
                        prdHeader.frameCount = maxFramesPerStack;
                    else
                        prdHeader.frameCount = ((saveFirst - 1) % maxFramesPerStack) + 1;

                    fileName += "ss_stack_first_";
                }
                else // doSaveLast
                {
                    if (stackIndex < (saveLast - 1) / maxFramesPerStack)
                        prdHeader.frameCount = maxFramesPerStack;
                    else
                        prdHeader.frameCount = ((saveLast - 1) % maxFramesPerStack) + 1;

                    fileName += "ss_stack_last_";
                }
                fileName += std::to_string(stackIndex);

                // Add proper file extension and create its instance
                switch (storageType)
                {
                case StorageType::Prd:
                    fileName += ".prd";
                    file = new(std::nothrow) PrdFileSave(fileName, prdHeader);
                    break;
                case StorageType::Tiff:
                    fileName += ".tiff";
                    file = new(std::nothrow) TiffFileSave(fileName, prdHeader);
                    break;
                case StorageType::None:
                    break;
                // No default section, compiler will complain when new format added
                }

                // Open the file
                if (!file || !file->Open())
                {
                    Log::LogE("Error in opening file %s for frame with index %llu",
                            fileName.c_str(), (unsigned long long)frameIndex);
                    keepGoing = false;

                    delete file;
                    file = nullptr;
                }
            }

            // If some file is open store current frame in it
            if (file)
            {
                const FRAME_INFO* fi = frame->GetFrameInfo();
                const uint64_t bof = (uint64_t)fi->TimeStampBOF * 100;
                const uint64_t eof = (uint64_t)fi->TimeStamp * 100;

                PrdMetaData metaData;
                memset(&metaData, 0, sizeof(PrdMetaData));
                // PRD v0.1 data
                metaData.frameNumber = (uint32_t)fi->FrameNr;
                metaData.readoutTime = (uint32_t)fi->ReadoutTime * 100;
                metaData.exposureTime =
                    (uint32_t)m_camera->GetSettings().GetExposure();
                // PRD v0.2 data
                metaData.bofTime = (uint32_t)(bof & 0xFFFFFFFF);
                metaData.eofTime = (uint32_t)(eof & 0xFFFFFFFF);
                // PRD v0.3 data
                metaData.roiCount =
                    (uint16_t)m_camera->GetSettings().GetRegions().size();
                // PRD v0.4 data
                metaData.bofTimeHigh = (uint32_t)((bof >> 32) & 0xFFFFFFFF);
                metaData.eofTimeHigh = (uint32_t)((eof >> 32) & 0xFFFFFFFF);

                if (!file->WriteFrame(&metaData, (uint16_t*)frame->GetData()))
                {
                    Log::LogE("Error in writing RAW data at %s for frame with index ",
                            fileName.c_str(), (unsigned long long)frameIndex);
                    keepGoing = false;
                }
                else
                {
                    m_toBeSavedFramesSaved++;
                }
            }
        }

        if (!keepGoing)
            RequestAbort();

        UnuseFrame(std::move(frame));

        frameIndex++;
    }

    // Just to be sure, close last file if remained open
    if (file)
    {
        file->Close();
        delete file;
    }
}

void pm::Acquisition::UpdateThreadLoop()
{
    const std::vector<std::string> progress{ "|", "/", "-", "\\" };
    size_t progressIndex = 0;

    while (!m_abortFlag && !m_updateThreadStopFlag)
    {
        // Use wait_for instead of sleep to stop immediately on request
        {
            std::unique_lock<std::mutex> lock(m_updateThreadMutex);
            m_updateThreadCond.wait_for(lock, std::chrono::milliseconds(500), [this]() {
                return (m_abortFlag || m_updateThreadStopFlag);
            });
        }
        if (m_abortFlag || m_updateThreadStopFlag)
            break;

        // Update limits not so often
        if (progressIndex == 0)
            UpdateToBeSavedFramesMax();

        // Print info about progress
        std::ostringstream ss;
        ss << progress[progressIndex] << " so far caught "
            << m_toBeProcessedFramesProcessed + m_toBeProcessedFramesLost
            << " frames";
        if (m_toBeProcessedFramesLost > 0)
            ss << " (" << m_toBeProcessedFramesLost << " of them lost)";
        ss << ", queued for saving " << m_toBeProcessedFramesProcessed
            << " frames";
        if (m_toBeSavedFramesLost > 0)
            ss << " (" << m_toBeSavedFramesLost << " of them dropped)";
        ss << ", saved " << m_toBeSavedFramesSaved << " frames";

        if (m_abortFlag)
            ss << ", aborting...";

        Log::LogP(ss.str());

        progressIndex = (progressIndex + 1) % progress.size();
    }
}

void pm::Acquisition::PrintAcqThreadStats() const
{
    const size_t frameCount = m_toBeProcessedFramesProcessed + m_toBeProcessedFramesLost;
    const double frameDropsPercent = (frameCount > 0)
        ? ((double)m_uncaughtFrames.GetCount() / (double)frameCount) * 100
        : 0;
    const double fps = (m_acqTime > 0)
        ? (double)m_toBeProcessedFramesProcessed / m_acqTime
        : 0;
    const double MiBps =
        round(fps * m_camera->GetFrameBytes() * 10 / 1024 / 1024) / 10.0;

    std::ostringstream ss;
    ss << "\nAcquisition thread queue stats:"
        << "\n    Frame count = " << frameCount
        << "\n  # Frame drops = " << m_uncaughtFrames.GetCount()
        << "\n  % Frame drops = " << frameDropsPercent
        << "\n  Average # frames between drops = " << m_uncaughtFrames.GetAvgSpacing()
        << "\n  Longest series of dropped frames = " << m_uncaughtFrames.GetLargestCluster()
        << "\n  Max. used frames = " << m_toBeProcessedFramesMaxPeak
        << " out of " << m_toBeProcessedFramesMax
        << "\n  Acquisition run with " << fps << " fps (~" << MiBps << "MiB/s)"
        << std::endl;

    if (m_outOfOrderFrameCount > 0)
    {
        ss << "\n  " << m_outOfOrderFrameCount
            << " frames with frame number <= last saved frame number"
            << std::endl;
    }

    Log::LogI(ss.str());
}

void pm::Acquisition::PrintDiskThreadStats() const
{
    const size_t frameCount = m_toBeSavedFramesSaved + m_toBeSavedFramesLost;
    const double frameDropsPercent = (frameCount > 0)
        ? ((double)m_unsavedFrames.GetCount() / (double)frameCount) * 100
        : 0;
    const double fps = (m_diskTime > 0)
        ? (double)m_toBeSavedFramesSaved / m_diskTime
        : 0;
    const double MiBps =
        round(fps * m_camera->GetFrameBytes() * 10 / 1024 / 1024) / 10.0;

    std::ostringstream ss;
    ss << "\nDisk thread queue stats:"
        << "\n    Frame count = " << frameCount
        << "\n  # Frame drops = " << m_unsavedFrames.GetCount()
        << "\n  % Frame drops = " << frameDropsPercent
        << "\n  Average # frames between drops = " << m_unsavedFrames.GetAvgSpacing()
        << "\n  Longest series of dropped frames = " << m_unsavedFrames.GetLargestCluster()
        << "\n  Max. used frames = " << m_toBeSavedFramesMaxPeak
        << " out of " << m_toBeSavedFramesMax
        << "\n  Saving run with " << fps << " fps (~" << MiBps << "MiB/s)"
        << std::endl;

    Log::LogI(ss.str());
}
