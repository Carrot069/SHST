#pragma once
#ifndef PM_FRAME_H
#define PM_FRAME_H

/* System */
#include <cstdlib>
#include <memory>

/* PVCAM */
#include <master.h>
#include <pvcam.h>

namespace pm {

class Frame
{
public:
    Frame(size_t frameBytes, bool deepCopy);
    ~Frame();

    Frame() = delete;
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    // Stores frame info and copies only pointer to data
    bool CopyDataPointer(const FRAME_INFO* frameInfo, void* data);
    /* Makes a deep copy with data pointer stored by CopyDataPointers.
       This is usually done by another thread and it is *not* responsibility
       of this class to watch the stored data pointer still points to valid
       data.
       If frame was created with isCopy false, the deep copy is not needed
       but you still should call it. */
    bool CopyData();

    std::shared_ptr<Frame> Clone(bool deepCopy = true) const;

    size_t GetFrameBytes() const
    { return m_frameBytes; }

    const void* GetData() const
    { return m_data; }

    const FRAME_INFO* GetFrameInfo() const
    { return &m_frameInfo; }

private:
    const bool m_deepCopy;
    const size_t m_frameBytes;
    void* m_data;
    void* m_dataSrc;
    FRAME_INFO m_frameInfo;
};

} // namespace pm

#endif /* PM_FRAME_H */
