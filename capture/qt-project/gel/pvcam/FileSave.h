#pragma once
#ifndef _FILE_SAVE_H
#define _FILE_SAVE_H

/* Local */
#include "File.h"
#include "PrdFileFormat.h"

namespace pm {

class FileSave : public File
{
public:
    FileSave(const std::string& fileName, PrdHeader& header);
    virtual ~FileSave();

    FileSave() = delete;
    FileSave(const FileSave&) = delete;
    FileSave& operator=(const FileSave&) = delete;

public:
    // New frame is added at end of the file
    virtual bool WriteFrame(const PrdMetaData* metaData, const void* rawData);

protected:
    PrdHeader& m_header;
    const size_t m_width;
    const size_t m_height;
    const size_t m_rawDataBytes;
};

} // namespace pm

#endif /* _FILE_SAVE_H */
