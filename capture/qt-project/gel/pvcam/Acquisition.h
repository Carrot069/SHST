#pragma once
#ifndef PM_ACQUISITION_H
#define PM_ACQUISITION_H

/* System */
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

/* Local */
#include "FpsLimiter.h"
#include "ListStatistics.h"
#include "PrdFileFormat.h"
#include "Timer.h"

namespace std
{
    class thread;
}

// Forward declaration for FRAME_INFO that satisfies compiler (taken from pvcam.h)
struct _TAG_FRAME_INFO;
typedef struct _TAG_FRAME_INFO FRAME_INFO;

namespace pm {

class Frame;
class Camera;

class Acquisition
{
public:
    static void EofCallback(FRAME_INFO* frameInfo, void* Acquisition_pointer);

public:
    Acquisition(std::shared_ptr<Camera> camera);
    ~Acquisition();

    Acquisition() = delete;
    Acquisition(const Acquisition&) = delete;
    Acquisition& operator=(const Acquisition&) = delete;

    // Starts the acquisition
    bool Start(std::shared_ptr<FpsLimiter> fpsLimiter = nullptr);
    // Returns true if acquisition is running, false otherwise
    bool IsRunning() const;
    // Forces correct acquisition interruption
    void RequestAbort();
    /* Blocks until the acquisition completes or reacts to abort request.
       Return true if stopped due to abort request. */
    bool WaitForStop(bool printStats = false);

private:
    // Allocates one new frame
    std::unique_ptr<Frame> AllocateNewFrame();

    // Put frame back to queue with unused frames
    void UnuseFrame(std::unique_ptr<Frame> frame);

    // Called from callback function to handle new frame
    bool HandleEofCallback(FRAME_INFO* frameInfo);
    // Called from AcqThreadLoop to handle new frame
    bool HandleNewFrame(std::unique_ptr<Frame> frame);

    // Updates max. allowed number of frames in queue to be saved
    void UpdateToBeSavedFramesMax();
    // Preallocate or release some ready-to-use frames at start/end
    bool PreallocateUnusedFrames();
    // Configures how frames will be stored on disk
    bool ConfigureStorage();

    // The function performs in m_acqThread, caches frames from camera
    void AcqThreadLoop();
    // The function performs in m_diskThread, saves frames to disk
    void DiskThreadLoop();
    // Called from DiskThreadLoop for one frame per file
    void DiskThreadLoop_Single();
    // Called from DiskThreadLoop for stacked frames in file
    void DiskThreadLoop_Stack();
    // The function performs in m_updateThread, saves frames to disk
    void UpdateThreadLoop();

    void PrintAcqThreadStats() const;
    void PrintDiskThreadStats() const;

private:
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<FpsLimiter> m_fpsLimiter;

    uint32_t m_maxFramesPerStack; // Limited to 32 bit

    // Uncaught frames statistics
    ListStatistics<size_t> m_uncaughtFrames;
    // Unsaved frames statistics
    ListStatistics<size_t> m_unsavedFrames;

    std::thread* m_acqThread;
    std::thread* m_diskThread;
    std::thread* m_updateThread;
    std::atomic<bool> m_abortFlag;

    // Time taken to finish acquisition, zero if in progress
    double m_acqTime;
    // Time taken to finish saving, zero if in progress
    double m_diskTime;

    int32_t m_lastFrameNumber; // Same type is in PVCAM FRAME_INFO::FrameNr

    std::atomic<size_t> m_outOfOrderFrameCount;

    // Mutex that guards all non-atomic m_updateThread* variables
    std::mutex              m_updateThreadMutex;
    // Condition the update thread waits on for new update iteration
    std::condition_variable m_updateThreadCond;
    // Stop flag that allows to stop update thread without aborting others
    std::atomic<bool>       m_updateThreadStopFlag;

    /*
       Data flow is like this:
       1. In callback handler is:
          - taken one frame from m_unusedFrames queue or allocated new Frame
            instance if queue is empty
          - stored frame info and pointer to data (shallow copy only) in frame
          - frame put to m_toBeProcessedFrames queue
       2. In acquisition thread is:
          - made deep copy of frame's data
          - done checks for lost frames
          - frame moved to m_toBeSavedFrames queue
       3. In disk thread is:
          - frame stored to disk in chosen format
          - frame moved back to m_unusedFrames queue
    */

    // Frames captured in callback thread to be processed in acquisition thread
    std::queue<std::unique_ptr<Frame>>  m_toBeProcessedFrames;
    // Mutex that guards all non-atomic m_capturedFrame* variables
    std::mutex                          m_toBeProcessedFramesMutex;
    // Condition the acquisition thread waits on for new frame
    std::condition_variable             m_toBeProcessedFramesCond;
    // Maximal size of queue with captured frames
    std::atomic<size_t>                 m_toBeProcessedFramesMax;
    // Highest number of frames that were ever stored in this queue
    std::atomic<size_t>                 m_toBeProcessedFramesMaxPeak;
    // Holds how many new frames have been lost
    std::atomic<size_t>                 m_toBeProcessedFramesLost;
    // Holds how many new frames have been processed
    std::atomic<size_t>                 m_toBeProcessedFramesProcessed;

    // Frames queued in acquisition thread to be saved to disk
    std::queue<std::unique_ptr<Frame>>  m_toBeSavedFrames;
    // Mutex that guards all non-atomic m_toBeSavedFrames* variables
    std::mutex                          m_toBeSavedFramesMutex;
    // Condition the frame saving thread waits on for new frame
    std::condition_variable             m_toBeSavedFramesCond;
    // Maximal size of queue with queued frames
    std::atomic<size_t>                 m_toBeSavedFramesMax;
    // Highest number of frames that were ever stored in m_toBeSavedFrames queue
    std::atomic<size_t>                 m_toBeSavedFramesMaxPeak;
    // Holds how many queued frames have not been saved to disk
    std::atomic<size_t>                 m_toBeSavedFramesLost;
    // Holds how many queued frames have been saved to disk
    std::atomic<size_t>                 m_toBeSavedFramesSaved;

    // Unused but allocated frames to be re-used
    std::queue<std::unique_ptr<Frame>>  m_unusedFrames;
    // Mutex that guards all non-atomic m_unusedFrames* variables
    std::mutex                          m_unusedFramesMutex;
};

} // namespace pm

#endif /* PM_ACQUISITION_H */
