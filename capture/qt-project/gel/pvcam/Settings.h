#pragma once
#ifndef PM_SETTINGS_H
#define PM_SETTINGS_H

/* System */
#include <string>
#include <vector>

/* PVCAM */
#include <master.h>
#include <pvcam.h>

/* Local */
#include "OptionController.h"
#include "PrdFileFormat.h"
#include "SettingsReader.h"

namespace pm {

class Settings : public SettingsReader
{
public:
    class ReadOnlyWriter
    {
        friend class Settings;

    private: // Do not allow creation from anywhere else than Settings class
        ReadOnlyWriter(Settings& settings);

        ReadOnlyWriter() = delete;
        ReadOnlyWriter(const ReadOnlyWriter&) = delete;
        ReadOnlyWriter& operator=(const ReadOnlyWriter&) = delete;

    public:
        bool SetBitDepth(uns16 value);
        bool SetWidth(uns16 value);
        bool SetHeight(uns16 value);
        bool SetCircBufferCapable(bool value);
        bool SetMetadataCapable(bool value);
        bool SetColorMask(int32 value);
        bool SetRegionCountMax(uns16 value);
        bool SetCentroidsCapable(bool value);
        bool SetCentroidCountMax(uns16 value);
        bool SetCentroidRadiusMax(uns16 value);

    private:
        Settings& m_settings;
    };

public:
    Settings();
    virtual ~Settings();

public:
    // Add all supported CLI options to given controller except Help option.
    // If you want to support Help option, add it separately.
    bool AddOptions(OptionController& controller);

    ReadOnlyWriter& GetReadOnlyWriter();

public: // To be called by anyone
    bool SetCamIndex(int16 value);

    bool SetPortIndex(int32 value);
    bool SetSpeedIndex(int16 value);

    bool SetGainIndex(int16 value);

    bool SetClrCycles(uns16 value);
    bool SetClrMode(int32 value);
    bool SetTrigMode(int32 value);
    bool SetExpOutMode(int32 value);
    bool SetMetadataEnabled(bool value);
    bool SetTrigTabSignal(int32 value);
    bool SetLastMuxedSignal(uns8 value);
    bool SetExposureResolution(int32 value);

    bool SetAcqFrameCount(uns32 value);
    bool SetBufferFrameCount(uns32 value);

    bool SetBinningSerial(uns16 value);
    bool SetBinningParallel(uns16 value);
    bool SetRegions(const std::vector<rgn_type>& value);

    bool SetExposure(uns32 value);
    bool SetAcqMode(AcqMode value);
    bool SetTimeLapseDelay(unsigned int value);

    bool SetStorageType(StorageType value);
    bool SetSaveDir(const std::string& value);
    bool SetSaveFirst(size_t value);
    bool SetSaveLast(size_t value);
    bool SetMaxStackSize(size_t value);

    bool SetCentroidsEnabled(bool value);
    bool SetCentroidCount(uns16 value);
    bool SetCentroidRadius(uns16 value);

private: // To be called by OptionController only (CLI options parsing)
    bool HandleCamIndex(const std::string& value);

    bool HandlePortIndex(const std::string& value);
    bool HandleSpeedIndex(const std::string& value);

    bool HandleGainIndex(const std::string& value);
    //bool HandleBitDepth(const std::string& value); // Not settable from CLI

    //bool HandleWidth(const std::string& value); // Not settable from CLI
    //bool HandleHeight(const std::string& value); // Not settable from CLI
    bool HandleClrCycles(const std::string& value);
    bool HandleClrMode(const std::string& value);
    bool HandleTrigMode(const std::string& value);
    bool HandleExpOutMode(const std::string& value);
    //bool HandleCircBufferCapable(const std::string& value); // Not settable from CLI
    //bool HandleMetadataCapable(const std::string& value); // Not settable from CLI
    bool HandleMetadataEnabled(const std::string& value);
    //bool HandleColorMask(const std::string& value); // Not settable from CLI
    bool HandleTrigTabSignal(const std::string& value);
    bool HandleLastMuxedSignal(const std::string& value);

    bool HandleAcqFrameCount(const std::string& value);
    bool HandleBufferFrameCount(const std::string& value);

    bool HandleBinningSerial(const std::string& value);
    bool HandleBinningParallel(const std::string& value);
    bool HandleRegions(const std::string& value);
    //bool HandleRegionCountMax(const std::string& value); // Not settable from CLI

    bool HandleExposure(const std::string& value);
    bool HandleAcqMode(const std::string& value);
    bool HandleTimeLapseDelay(const std::string& value);

    bool HandleStorageType(const std::string& value);
    bool HandleSaveDir(const std::string& value);
    bool HandleSaveFirst(const std::string& value);
    bool HandleSaveLast(const std::string& value);
    bool HandleMaxStackSize(const std::string& value);

    //bool HandleCentroidsCapable(const std::string& value); // Not settable from CLI
    bool HandleCentroidsEnabled(const std::string& value);
    bool HandleCentroidCount(const std::string& value);
    //bool HandleCentroidCountMax(const std::string& value); // Not settable from CLI
    bool HandleCentroidRadius(const std::string& value);
    //bool HandleCentroidRadiusMax(const std::string& value); // Not settable from CLI

protected:
    ReadOnlyWriter m_readOnlyWriter;
};

} // namespace pm

#endif /* PM_SETTINGS_H */
