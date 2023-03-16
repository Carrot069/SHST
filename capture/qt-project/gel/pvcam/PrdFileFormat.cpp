#include "PrdFileFormat.h"

/* System */
#include <cstring>

void ClearPrdHeaderStructure(PrdHeader& header)
{
    memset(&header, 0, sizeof(PrdHeader));
    header.type[0] = 'P';
    header.type[1] = 'R';
    header.type[2] = 'D';
    header.type[3] = '\0';
}

size_t GetRawDataSizeInBytes(const PrdHeader& header)
{
    const PrdRegion& region = header.region;
    if (region.sbin == 0 || region.pbin == 0)
        return 0;
    size_t bytes;
    if (header.version >= PRD_VERSION_0_3)
    {
        bytes = header.frameSize;
    }
    else
    {
        const uint16_t width = (region.s2 - region.s1 + 1) / region.sbin;
        const uint16_t height = (region.p2 - region.p1 + 1) / region.pbin;
        bytes = sizeof(uint16_t) * width * height;
    }
    return bytes;
}

size_t GetPrdFileSizeOverheadInBytes(const PrdHeader& header)
{
    return sizeof(PrdHeader) + header.frameCount * header.sizeOfPrdMetaDataStruct;
}

size_t GetPrdFileSizeInBytes(const PrdHeader& header)
{
    const size_t rawDataBytes = GetRawDataSizeInBytes(header);
    if (rawDataBytes == 0)
        return 0;
    return GetPrdFileSizeOverheadInBytes(header) + header.frameCount * rawDataBytes;
}

uint32_t GetFrameCountThatFitsIn(const PrdHeader& header, size_t maxSizeInBytes)
{
    const size_t rawDataBytes = GetRawDataSizeInBytes(header);
    if (rawDataBytes == 0 || maxSizeInBytes <= sizeof(PrdHeader))
        return 0;
    return (uint32_t)((maxSizeInBytes - sizeof(PrdHeader))
        / (header.sizeOfPrdMetaDataStruct + rawDataBytes));
}
