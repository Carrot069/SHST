#include "FileSave.h"

pm::FileSave::FileSave(const std::string& fileName, PrdHeader& header)
    : File(fileName),
    m_header(header),
    m_width((header.region.sbin == 0)
        ? 0
        : (header.region.s2 - header.region.s1 + 1) / header.region.sbin),
    m_height((header.region.pbin == 0)
        ? 0
        : (header.region.p2 - header.region.p1 + 1) / header.region.pbin),
    m_rawDataBytes(GetRawDataSizeInBytes(header))
{
}

pm::FileSave::~FileSave()
{
}

bool pm::FileSave::WriteFrame(const PrdMetaData* metaData, const void* rawData)
{
    if (!metaData || !rawData || m_width == 0 || m_height == 0 || m_rawDataBytes == 0)
        return false;

    return true;
}
