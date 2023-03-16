#pragma once
#ifndef _PRD_FILE_FORMAT_H
#define _PRD_FILE_FORMAT_H

/* System */
#include <cstdint>
#include <cstdlib>

/* TODO: Current implementation supports PRD format on architectures
         with little endian only. Remove that limitation. */

/** PRD file versions.
    Higher version must have higher number assigned.
    @{ */
/// PRD version 0.1
#define PRD_VERSION_0_1 ((uint16_t)0x0001)
/// PRD version 0.2
#define PRD_VERSION_0_2 ((uint16_t)0x0002)
/// PRD version 0.3
#define PRD_VERSION_0_3 ((uint16_t)0x0003)
/// PRD version 0.4
#define PRD_VERSION_0_4 ((uint16_t)0x0004)
/** @} */

/** PRD exposure resolutions.
    @{ */
/// Exposure resolution in microseconds.
#define PRD_EXP_RES_US  ((uint32_t)1)
/// Exposure resolution in milliseconds.
#define PRD_EXP_RES_MS  ((uint32_t)1000)
/// Exposure resolution in seconds.
#define PRD_EXP_RES_S   ((uint32_t)1000000)
/** @} */

/** PRD frame flags (bits).
    @{ */
/// Frame has metadata
#define PRD_FLAG_HAS_METADATA ((uint8_t)0x01)
/** @} */

// Deny compiler to align these structures
#pragma pack(push)
#pragma pack(1)

/// Structure describing the area and binning factor used for acquisition.
/// PrdRegion type is compatible with PVCAM rgn_type type.
typedef struct PrdRegion // 12 bytes
{
    /// First serial/horizontal pixel.
    uint16_t s1; // 2 bytes
    /// Last serial/horizontal pixel.
    /// Must be equal or greater than s1.
    uint16_t s2; // 2 bytes
    /// Serial/horizontal binning.
    /// Must not be zero.
    uint16_t sbin; // 2 bytes
    /// First parallel/vertical pixel.
    uint16_t p1; // 2 bytes
    /// Last parallel/vertical pixel.
    /// Must be equal or greater than p1.
    uint16_t p2; // 2 bytes
    /// Parallel/vertical binning.
    /// Must not be zero.
    uint16_t pbin; // 2 bytes
}
PrdRegion;

/// Detailed information about captured frame.
typedef struct PrdMetaData // 40 bytes
{
    /** Members introduced in PRD_VERSION_0_1
        @{ */

    /// Frame index, should be unique and first is 1.
    uint32_t frameNumber; // 4 bytes
    /// Readout time in microseconds (does not include exposure time).
    uint32_t readoutTime; // 4 bytes

    /// Exposure time in micro-, milli- or seconds, depends on exposureResolution.
    uint32_t exposureTime; // 4 bytes

    /** @} */

    /** Members introduced in PRD_VERSION_0_2
        @{ */

    /// BOF time in microseconds (taken from acquisition start).
    uint32_t bofTime; // 4 bytes
    /// EOF time in microseconds (taken from acquisition start).
    uint32_t eofTime; // 4 bytes

    /** @} */

    /** Members introduced in PRD_VERSION_0_3
        @{ */

    /// ROI count (1 for frames without PRD_FLAG_HAS_METADATA flag).
    uint16_t roiCount; // 2 bytes

    /** @} */

    /** Members introduced in PRD_VERSION_0_4
        @{ */

    /// Upper 4 byte of BOF time in microseconds (taken from acquisition start).
    uint32_t bofTimeHigh; // 4 bytes
    /// Upper 4 byte of EOF time in microseconds (taken from acquisition start).
    uint32_t eofTimeHigh; // 4 bytes

    /** @} */

    /// Reserved space used only for structure alignment at the moment.
    uint8_t _reserved[10]; // 10 bytes
}
PrdMetaData;

/// PRD (Photometrics Raw Data) file format.
/** Numbers in all structures are stored in little endian.
    PRD file consists of:
    - PrdHeader structure
    - PrdHeader.frameCount times repeated following data:
        - PrdMetaData structure
        - RAW pixel data, optionally with metadata, always 2 bytes per pixel */
// The size of PrdHeader should stay 48 bytes and never change!
typedef struct PrdHeader // 48 bytes
{
    /** Members introduced in PRD_VERSION_0_1
        @{ */

    /// Contains null-terminated string "PRD".
    char type[4]; // 4 bytes

    /// Contains one of PRD_VERSION_* macro values.
    uint16_t version; // 2 bytes
    /// Raw data bit depth taken from camera (but every pixel is stored in 16 bits).
    uint16_t bitDepth; // 2 bytes

    /// Usually 1, but for stack might be greater than 1.
    uint32_t frameCount; // 4 bytes

    /// Used chip region in pixels and binning.
    /** This region can have a bit different meaning depending on file version
        and metadata.
        - Frame without PVCAM metadata - Only one ROI can be setup for
            acquisition which is directly stored in here.
        - Frame with PVCAM metadata (supported since PRD_VERSION_0_3)
            - Multi-ROI frame - The frame consists of multiple static regions
                specified by user. In this case #region specifies calculated
                implied ROI containing all given regions.
            - Frame with centroids - The frame consists of multiple small and
                dynamically generated regions (by camera). With centroids only
                one ROI can be setup for acquisition by user which is directly
                stored in here. Please note that the implied ROI as stored in
                PVCAM metadata structures is not the same and is usually within
                that #region.
        Anyway, it always defines the dimensions of final image reconstructed
        from raw data.
        Calculate image width  (uint16_t) from region as: (s2 - s1 + 1) / sbin.
        Calculate image height (uint16_t) from region as: (p2 - p1 + 1) / pbin.
         */
    PrdRegion region; // 12 bytes

    /// Size of PrdMetaData structure used while saving.
    uint32_t sizeOfPrdMetaDataStruct; // 4 bytes

    /// Exposure resolution.
    /// Is one of PRD_EXP_RES_* macro values.
    uint32_t exposureResolution; // 4 bytes

    /** @} */

    /** Members introduced in PRD_VERSION_0_3
        @{ */

    /// Color mask (correcponds to PL_COLOR_MODES).
    uint8_t colorMask; // 1 byte
    /// Contains one of PRD_FLAG_* macro values.
    uint8_t flags; // 1 byte

    /// Size of frame raw data in bytes.
    /** For frame without metadata the size can be calculated from #region.
        Size of the frame with metadata depends on number of ROIs/centroids,
        extended metadata size, etc. */
    uint32_t frameSize; // 4 bytes

    /** @} */

    /// Reserved space used only for structure alignment at the moment.
    uint8_t _reserved[10]; // 10 bytes
}
PrdHeader;

// Restore default alignment
#pragma pack(pop)

/// Function initializes PrdHeader structure with zeroes and sets its type member.
void ClearPrdHeaderStructure(PrdHeader& header);

/// Calculates RAW data size.
/** It requires only following header members: region. */
size_t GetRawDataSizeInBytes(const PrdHeader& header);

/// Calculates PRD file data overhead from its header.
/** It requires only following header members: frameCount and sizeOfPrdMetaDataStruct. */
size_t GetPrdFileSizeOverheadInBytes(const PrdHeader& header);

/// Calculates size of whole PRD file from its header.
/** It requires only following header members: region, frameCount and sizeOfPrdMetaDataStruct. */
size_t GetPrdFileSizeInBytes(const PrdHeader& header);

/// Calculates max. number of frames in PRD file that fits into given limit.
/** It requires only following header members: region and sizeOfPrdMetaDataStruct. */
uint32_t GetFrameCountThatFitsIn(const PrdHeader& header, size_t maxSizeInBytes);

#endif /* _PRD_FILE_FORMAT_H */
