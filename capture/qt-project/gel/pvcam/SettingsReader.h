#pragma once
#ifndef PM_SETTINGS_READER_H
#define PM_SETTINGS_READER_H

/* System */
#include <cstdint>
#include <string>
#include <vector>

/* PVCAM */
#include <master.h>
#include <pvcam.h>

namespace pm {

enum class AcqMode : int32_t
{
    SnapSequence,
    SnapCircBuffer,
    SnapTimeLapse,
    LiveCircBuffer,
    LiveTimeLapse,
};

enum class StorageType : int32_t
{
    None,
    Prd,
    Tiff,
};

class SettingsReader
{
public:
    SettingsReader();
    virtual ~SettingsReader();

public:
    static rgn_type GetImpliedRegion(const std::vector<rgn_type>& regions);

public:
    int16 GetCamIndex() const
    { return m_camIndex; }

    int32 GetPortIndex() const
    { return m_portIndex; }
    int16 GetSpeedIndex() const
    { return m_speedIndex; }

    int16 GetGainIndex() const
    { return m_gainIndex; }
    uns16 GetBitDepth() const
    { return m_bitDepth; }

    uns16 GetWidth() const
    { return m_width; }
    uns16 GetHeight() const
    { return m_height; }
    uns16 GetClrCycles() const
    { return m_clrCycles; }
    int32 GetClrMode() const
    { return m_clrMode; }
    int32 GetTrigMode() const
    { return m_trigMode; }
    int32 GetExpOutMode() const
    { return m_expOutMode; }
    bool GetCircBufferCapable() const
    { return m_circBufferCapable; }
    bool GetMetadataCapable() const
    { return m_metadataCapable; }
    bool GetMetadataEnabled() const
    { return m_metadataEnabled; }
    int32 GetColorMask() const
    { return m_colorMask; }
    int32 GetTrigTabSignal() const
    { return m_trigTabSignal; }
    uns8 GetLastMuxedSignal() const
    { return m_lastMuxedSignal; }

    uns32 GetAcqFrameCount() const
    { return m_acqFrameCount; }
    uns32 GetBufferFrameCount() const
    { return m_bufferFrameCount; }

    uns16 GetBinningSerial() const
    { return m_binSer; }
    uns16 GetBinningParallel() const
    { return m_binPar; }
    const std::vector<rgn_type>& GetRegions() const
    { return m_regions; }
    uns16 GetRegionCountMax() const
    { return m_regionCountMax; }

    uns32 GetExposure() const
    { return m_expTime; }
    int32 GetExposureResolution() const
    { return m_expTimeRes; }
    AcqMode GetAcqMode() const
    { return m_acqMode; }
    unsigned int GetTimeLapseDelay() const
    { return m_timeLapseDelay; }

    StorageType GetStorageType() const
    { return m_storageType; }
    const std::string& GetSaveDir() const
    { return m_saveDir; }
    size_t GetSaveFirst() const
    { return m_saveFirst; }
    size_t GetSaveLast() const
    { return m_saveLast; }
    size_t GetMaxStackSize() const
    { return m_maxStackSize; }

    bool GetCentroidsCapable() const
    { return m_centroidsCapable; }
    bool GetCentroidsEnabled() const
    { return m_centroidsEnabled; }
    uns16 GetCentroidCount() const
    { return m_centroidCount; }
    uns16 GetCentroidCountMax() const
    { return m_centroidCountMax; }
    uns16 GetCentroidRadius() const
    { return m_centroidRadius; }
    uns16 GetCentroidRadiusMax() const
    { return m_centroidRadiusMax; }

protected:
    int16 m_camIndex;

    // PVCAM parameters defining speed table
    int32 m_portIndex;
    int16 m_speedIndex;

    // PVCAM parameters depending on speed table settings
    int16 m_gainIndex;
    uns16 m_bitDepth; // Read-only

    // Other PVCAM parameters
    uns16 m_width; // Read-only
    uns16 m_height; // Read-only
    uns16 m_clrCycles;
    int32 m_clrMode; // PL_CLEAR_MODES
    int32 m_trigMode; // PL_EXPOSURE_MODES
    int32 m_expOutMode; // PL_EXPOSE_OUT_MODES
    bool m_circBufferCapable; // Read-only
    bool m_metadataCapable; // Read-only
    bool m_metadataEnabled;
    int32 m_colorMask; // Read-only, PL_COLOR_MASKS
    int32 m_trigTabSignal; // PL_TRIGTAB_SIGNALS
    uns8 m_lastMuxedSignal;
    int32 m_expTimeRes; // PL_EXP_RES_MODES

    // Other non-PVCAM and combined parameters
    uns32 m_acqFrameCount;
    uns32 m_bufferFrameCount;

    uns16 m_binSer;
    uns16 m_binPar;
    std::vector<rgn_type> m_regions;
    uns16 m_regionCountMax; // Read-only

    uns32 m_expTime;
    AcqMode m_acqMode;
    unsigned int m_timeLapseDelay;

    StorageType m_storageType;
    std::string m_saveDir;
    size_t m_saveFirst;
    size_t m_saveLast;
    size_t m_maxStackSize;

    bool m_centroidsCapable;
    bool m_centroidsEnabled;
    uns16 m_centroidCount;
    uns16 m_centroidCountMax;
    uns16 m_centroidRadius;
    uns16 m_centroidRadiusMax;
};

} // namespace pm

#endif /* PM_SETTINGS_READER_H */
