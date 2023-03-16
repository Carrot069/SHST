#include "PrdFileSave.h"

/* System */
#include <cstring>

pm::PrdFileSave::PrdFileSave(const std::string& fileName, PrdHeader& header)
    : FileSave(fileName, header),
    m_file()
{
}

pm::PrdFileSave::~PrdFileSave()
{
    if (IsOpen())
        Close();
}

bool pm::PrdFileSave::Open()
{
    if (IsOpen())
        return true;

    m_file.open(m_fileName, std::ios_base::out | std::ios_base::binary);
    if (!m_file.is_open())
        return false;

    m_file.write((char*)&m_header, sizeof(PrdHeader));

    m_frameIndex = 0;

    return IsOpen();
}

bool pm::PrdFileSave::IsOpen() const
{
    return m_file.is_open();
}

void pm::PrdFileSave::Close()
{
    if (m_header.frameCount != m_frameIndex)
    {
        m_header.frameCount = m_frameIndex;

        m_file.seekp(0);
        m_file.write((char*)&m_header, sizeof(PrdHeader));
        m_file.seekp(0, std::ios_base::end);
    }

    m_file.flush();
    m_file.close();
}

bool pm::PrdFileSave::WriteFrame(const PrdMetaData* metaData, const void* rawData)
{
    if (!FileSave::WriteFrame(metaData, rawData))
        return false;

    m_file.write(reinterpret_cast<const char*>(metaData), m_header.sizeOfPrdMetaDataStruct);
    m_file.write(reinterpret_cast<const char*>(rawData), m_rawDataBytes);

    m_frameIndex++;
    return true;
}
