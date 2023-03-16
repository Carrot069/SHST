#include "Frame.h"

/* System */
#include <cstring>
#include <new>

pm::Frame::Frame(size_t frameBytes, bool deepCopy)
    : m_deepCopy(deepCopy),
    m_frameBytes(frameBytes),
    m_data(nullptr),
    m_dataSrc(nullptr),
    m_frameInfo()
{
    if (m_deepCopy)
        m_data = (void*)new(std::nothrow) uns8[m_frameBytes];

    memset(&m_frameInfo, 0, sizeof(m_frameInfo));
}

pm::Frame::~Frame()
{
    if (m_deepCopy)
        delete [] (uns8*)m_data;
}

bool pm::Frame::CopyDataPointer(const FRAME_INFO* frameInfo, void* data)
{
    if (!frameInfo)
        return false;

    memcpy(&m_frameInfo, frameInfo, sizeof(m_frameInfo));
    m_dataSrc = data;

    return true;
}

bool pm::Frame::CopyData()
{
    if (!m_dataSrc)
        return false;

    if (m_deepCopy)
    {
        if (!m_data)
            return false;
        memcpy(m_data, m_dataSrc, m_frameBytes);
    }
    else
    {
        m_data = m_dataSrc;
    }

    return true;
}

std::shared_ptr<pm::Frame> pm::Frame::Clone(bool deepCopy) const
{
    auto frame = std::make_shared<Frame>(m_frameBytes, deepCopy);

    frame->CopyDataPointer(&m_frameInfo, m_data);
    frame->CopyData();

    return frame;
}
