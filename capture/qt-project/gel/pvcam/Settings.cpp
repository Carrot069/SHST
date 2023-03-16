#include "Settings.h"

/* Local */
#include "Log.h"
#include "Utils.h"

using SettingsProto = bool(pm::Settings::*)(const std::string&);

// pm::Settings::ReadOnlyWriter

pm::Settings::ReadOnlyWriter::ReadOnlyWriter(Settings& settings)
    : m_settings(settings)
{
}

bool pm::Settings::ReadOnlyWriter::SetBitDepth(uns16 value)
{
    m_settings.m_bitDepth = value;
    return true;
}

bool pm::Settings::ReadOnlyWriter::SetWidth(uns16 value)
{
    m_settings.m_width = value;
    return true;
}

bool pm::Settings::ReadOnlyWriter::SetHeight(uns16 value)
{
    m_settings.m_height = value;
    return true;
}

bool pm::Settings::ReadOnlyWriter::SetCircBufferCapable(bool value)
{
    m_settings.m_circBufferCapable = value;
    return true;
}

bool pm::Settings::ReadOnlyWriter::SetMetadataCapable(bool value)
{
    m_settings.m_metadataCapable = value;
    return true;
}

bool pm::Settings::ReadOnlyWriter::SetColorMask(int32 value)
{
    m_settings.m_colorMask = value;
    return true;
}

bool pm::Settings::ReadOnlyWriter::SetRegionCountMax(uns16 value)
{
    m_settings.m_regionCountMax = value;
    return true;
}

bool pm::Settings::ReadOnlyWriter::SetCentroidsCapable(bool value)
{
    m_settings.m_centroidsCapable = value;
    return true;
}

bool pm::Settings::ReadOnlyWriter::SetCentroidCountMax(uns16 value)
{
    m_settings.m_centroidCountMax = value;
    return true;
}

bool pm::Settings::ReadOnlyWriter::SetCentroidRadiusMax(uns16 value)
{
    m_settings.m_centroidRadiusMax = value;
    return true;
}

// pm::Settings

pm::Settings::Settings()
    : SettingsReader(),
    m_readOnlyWriter(*this)
{
}

pm::Settings::~Settings()
{
}

bool pm::Settings::AddOptions(OptionController& controller)
{
    const char valSep = pm::Option::ValuesSeparator;
    const char grpSep = pm::Option::ValueGroupsSeparator;

    if (!controller.AddOption(pm::Option(
            { "-CamIndex", "--cam-index", "-c" },
            { "index" },
            { "0" },
            "Index of camera to be used for acquisition.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleCamIndex),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-PortIndex", "--port-index" },
            { "index" },
            { "0" },
            "Port index (first is 0).",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandlePortIndex),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-SpeedIndex", "--speed-index" },
            { "index" },
            { "0" },
            "Speed index (first is 0).",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleSpeedIndex),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-GainIndex", "--gain-index" },
            { "index" },
            { "1" },
            "Gain index (first is 1).",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleGainIndex),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-ClearCycles", "--clear-cycles" },
            { "count" },
            { "2" },
            "Number of clear cycles.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleClrCycles),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-ClearMode", "--clear-mode" },
            { "mode" },
            { "pre-exp" },
            "Clear mode used for sensor clearing during acquisition.\n"
            "Supported values are : 'never', 'pre-exp', 'pre-seq', 'post-seq',\n"
            "'pre-post-seq' and 'pre-exp-post-seq'.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleClrMode),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-TrigMode", "--trig-mode" },
            { "mode" },
            { "timed" },
            "Trigger (or exposure) mode used for exposure triggering.\n"
            "It is related to expose out mode, see details in PVCAM manual.\n"
            "Supported values are : Classics modes 'times', 'strobed', 'bulb',\n"
            "'trigger-first', 'flash', ('variable-timed'), 'int-strobe'\n"
            "and extended modes 'ext-internal', 'ext-trig-first' and 'ext-edge-raising'.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleTrigMode),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-ExpOutMode", "--exp-out-mode" },
            { "mode" },
            { "first-row" },
            "Expose mode used for exposure triggering.\n"
            "It is related to exposure mode, see details in PVCAM manual.\n"
            "Supported values are : 'first-row', 'all-rows', 'any-row' and 'rolling-shutter'.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleExpOutMode),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-UseMetadata", "--use-metadata" },
            {  },
            {  },
            "If camera supports frame metadata use it even if not needed.\n"
            "Application may silently override this value when metadata is needed,\n"
            "for instance multiple regions or centroids.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleMetadataEnabled),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-TrigtabSignal", "--trigtab-signal" },
            { "signal" },
            { "expose-out" },
            "The output signal with embedded multiplexer forwarding chosen signal\n"
            "to multiple output wires (set via --last-muxed-signal).\n"
            "Supported values are : 'expose-out'.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleTrigTabSignal),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-LastMuxedSignal", "--last-muxed-signal" },
            { "number" },
            { "1" },
            "Number of multiplexed output wires for chosen signal (set via --trigtab-signal).",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleLastMuxedSignal),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-AcqFrames", "--acq-frames", "-f" },
            { "count" },
            { "1" },
            "Total number of frames to be captured in acquisition.\n"
            "In snap sequence mode (set via --acq-mode) the total number of frames\n"
            "is limited to value 65535.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleAcqFrameCount),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-BufferFrames", "--buffer-frames" },
            { "count" },
            { "10" },
            "Number of frames in PVCAM circular buffer.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleBufferFrameCount),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-BinningSerial", "--binning-serial", "--sbin" },
            { "factor" },
            { "1" },
            "Serial binning factor.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleBinningSerial),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-BinningParallel", "--binning-parallel", "--pbin" },
            { "factor" },
            { "1" },
            "Parallel binning factor.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleBinningParallel),
                    this, std::placeholders::_1))))
        return false;

    const std::string roiArgsDescs = std::string()
        + "sA1" + valSep + "sA2" + valSep + "pA1" + valSep + "pA2" + grpSep
        + "sB1" + valSep + "sB2" + valSep + "pB1" + valSep + "pB2" + grpSep
        + "...";
    if (!controller.AddOption(pm::Option(
            { "--region", "--regions", "--rois", "--roi", "-r" },
            { roiArgsDescs },
            { "" },
            "Region of interest for serial (width) and parallel (height) dimension.\n"
            "'sA1' is the first pixel, 'sA2' is the last pixel of the first region\n"
            "included on row. The same applies to columns. Multiple regions are\n"
            "separated by semicolon.\n"
            "Example of two diagonal regions 10x10: \"--rois=0,9,0,9;10,19,10,19\".\n"
            "Binning factors are configured separately (via --sbin and --pbin).\n"
            "The empty value causes the camera's full-frame will be used internally.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleRegions),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-Exposure", "--exposure", "-e" },
            { "units" },
            { "10" },
            "Exposure time for each frame in millisecond units by default.\n"
            "Use us, ms or s suffix to change exposure resolution, e.g. 100us or 10ms.\n",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleExposure),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-AcqMode", "--acq-mode" },
            { "mode" },
            { "snap-circ-buffer" },
            "Specifies acquisition mode used for collecting images.\n"
            "Supported values are : 'snap-seq', 'snap-circ-buffer', 'snap-time-lapse',\n"
            "'live-circ-buffer' and 'live-time-lapse'.\n"
            "'snap-seq' mode:\n"
            "  Frames are captured in one sequence instead of continuous\n"
            "  acquisition with circular buffer.\n"
            "  Number of frames in buffer (set using --buffer-frames) has to\n"
            "  be equal or greater than number of frames in sequence\n"
            "  (set using --acq-frames).\n"
            "'snap-circ-buffer' mode:\n"
            "  Uses circular buffer to snap given number of frames in continuous\n"
            "  acquisition.\n"
            "  If the frame rate is high enough, it happens that number of\n"
            "  acquired frames is higher that requested, because new frames\n"
            "  can come between stop request and actual acq. interruption.\n"
            "'snap-time-lapse' mode:\n"
            "  Required number of frames is collected using multiple sequence\n"
            "  acquisitions where only one frame is captured at a time.\n"
            "  Delay between single frames can be set using --time-lapse-delay\n"
            "  option."
            "'live-circ-buffer' mode:\n"
            "  Uses circular buffer to snap frames in infinite continuous\n"
            "  acquisition.\n"
            "'live-time-lapse' mode:\n"
            "  The same as 'snap-time-lapse' but runs in infinite loop.\n",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleAcqMode),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-TimeLapseDelay", "--time-lapse-delay" },
            { "milliseconds" },
            { "0" },
            "A delay between single frames in time lapse mode.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleTimeLapseDelay),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-SaveAs", "--save-as" },
            { "format" },
            { "none" },
            "Stores captured frames on disk in chosen format.\n"
            "Supported values are: 'none', 'prd' and 'tiff'.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleStorageType),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-SaveDir", "--save-dir" },
            { "folder" },
            { "" },
            "Stores captured frames on disk in given directory.\n"
            "If empty string is given (the default) current working directory is used.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleSaveDir),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-SaveFirst", "--save-first" },
            { "count" },
            { "0" },
            "Saves only first <count> frames.\n"
            "If both --save-first and --save-last are zero, all frames are stored unless\n"
            "an option --save-as is 'none'.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleSaveFirst),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-SaveLast", "--save-last" },
            { "count" },
            { "0" },
            "Saves only last <count> frames.\n"
            "If both --save-first and --save-last are zero, all frames are stored unless\n"
            "an option --save-as is 'none'.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleSaveLast),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-MaxStackSize", "--max-stack-size" },
            { "size" },
            { "0" },
            "Stores multiple frames in one file up to given size.\n"
            "Another stack file with new index is created for more frames.\n"
            "Use k, M or G suffix to enter nicer values. (1k = 1024)\n"
            "Default value is 0 which means each frame is stored to its own file.\n"
            "WARNING:\n"
            "  Storing too many small frames into one TIFF file (using --max-stack-size)\n"
            "  might be significantly slower compared to PRD format!",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleMaxStackSize),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-UseCentroids", "--use-centroids" },
            {  },
            {  },
            "Turns on the centroids feature.\n"
            "This feature can be used with up to one region only.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleCentroidsEnabled),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-CentroidCount", "--centroid-count" },
            { "count" },
            { "100" },
            "Requests camera to find given number of centroids.\n"
            "Application may override this value if it is greater than max. number of\n"
            "supported centroids.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleCentroidCount),
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(pm::Option(
            { "-CentroidRadius", "--centroid-radius" },
            { "radius" },
            { "15" },
            "Specifies the radius of all centroids.",
            std::bind(static_cast<SettingsProto>(&pm::Settings::HandleCentroidRadius),
                    this, std::placeholders::_1))))
        return false;

    return true;
}

pm::Settings::ReadOnlyWriter& pm::Settings::GetReadOnlyWriter()
{
    return m_readOnlyWriter;
}

bool pm::Settings::SetCamIndex(int16 value)
{
    m_camIndex = value;
    return true;
}

bool pm::Settings::SetPortIndex(int32 value)
{
    m_portIndex = value;
    return true;
}

bool pm::Settings::SetSpeedIndex(int16 value)
{
    m_speedIndex = value;
    return true;
}

bool pm::Settings::SetGainIndex(int16 value)
{
    m_gainIndex = value;
    return true;
}

bool pm::Settings::SetClrCycles(uns16 value)
{
    m_clrCycles = value;
    return true;
}

bool pm::Settings::SetClrMode(int32 value)
{
    m_clrMode = value;
    return true;
}

bool pm::Settings::SetTrigMode(int32 value)
{
    m_trigMode = value;
    return true;
}

bool pm::Settings::SetExpOutMode(int32 value)
{
    m_expOutMode = value;
    return true;
}

bool pm::Settings::SetMetadataEnabled(bool value)
{
    m_metadataEnabled = value;
    return true;
}

bool pm::Settings::SetTrigTabSignal(int32 value)
{
    m_trigTabSignal = value;
    return true;
}

bool pm::Settings::SetLastMuxedSignal(uns8 value)
{
    m_lastMuxedSignal = value;
    return true;
}

bool pm::Settings::SetExposureResolution(int32 value)
{
    switch (value)
    {
    case EXP_RES_ONE_MICROSEC:
    case EXP_RES_ONE_MILLISEC:
    case EXP_RES_ONE_SEC:
        m_expTimeRes = value;
        return true;
    default:
        return false;
    }
}

bool pm::Settings::SetAcqFrameCount(uns32 value)
{
    m_acqFrameCount = value;
    return true;
}

bool pm::Settings::SetBufferFrameCount(uns32 value)
{
    m_bufferFrameCount = value;
    return true;
}

bool pm::Settings::SetBinningSerial(uns16 value)
{
    if (value == 0)
        return false;

    m_binSer = value;

    for (size_t n = 0; n < m_regions.size(); n++)
    {
        m_regions[n].sbin = m_binSer;
        m_regions[n].pbin = m_binPar;
    }

    return true;
}

bool pm::Settings::SetBinningParallel(uns16 value)
{
    if (value == 0)
        return false;

    m_binPar = value;

    for (size_t n = 0; n < m_regions.size(); n++)
    {
        m_regions[n].sbin = m_binSer;
        m_regions[n].pbin = m_binPar;
    }

    return true;
}

bool pm::Settings::SetRegions(const std::vector<rgn_type>& value)
{
    for (auto roi : value)
    {
        if (roi.sbin != m_binSer || roi.pbin != m_binPar)
        {
            Log::LogE("Region binning factors do not match");
            return false;
        }
    }

    m_regions = value;
    return true;
}

bool pm::Settings::SetExposure(uns32 value)
{
    m_expTime = value;
    return true;
}

bool pm::Settings::SetAcqMode(AcqMode value)
{
    m_acqMode = value;
    return true;
}

bool pm::Settings::SetTimeLapseDelay(unsigned int value)
{
    m_timeLapseDelay = value;
    return true;
}

bool pm::Settings::SetStorageType(StorageType value)
{
    m_storageType = value;
    return true;
}

bool pm::Settings::SetSaveDir(const std::string& value)
{
    m_saveDir = value;
    return true;
}

bool pm::Settings::SetSaveFirst(size_t value)
{
    m_saveFirst = value;
    return true;
}

bool pm::Settings::SetSaveLast(size_t value)
{
    m_saveLast = value;
    return true;
}

bool pm::Settings::SetMaxStackSize(size_t value)
{
    m_maxStackSize = value;
    return true;
}

bool pm::Settings::SetCentroidsEnabled(bool value)
{
    m_centroidsEnabled = value;
    return true;
}

bool pm::Settings::SetCentroidCount(uns16 value)
{
    m_centroidCount = value;
    return true;
}

bool pm::Settings::SetCentroidRadius(uns16 value)
{
    m_centroidRadius = value;
    return true;
}

bool pm::Settings::HandleCamIndex(const std::string& value)
{
    uns16 index;
    if (!StrToNumber<uns16>(value, index))
        return false;
    if (index >= std::numeric_limits<int16>::max())
        return false;

    return SetCamIndex((int16)index);
}

bool pm::Settings::HandlePortIndex(const std::string& value)
{
    uns32 portIndex;
    if (!StrToNumber<uns32>(value, portIndex))
        return false;

    return SetPortIndex(portIndex);
}

bool pm::Settings::HandleSpeedIndex(const std::string& value)
{
    int16 speedIndex;
    if (!StrToNumber<int16>(value, speedIndex))
        return false;

    return SetSpeedIndex(speedIndex);
}

bool pm::Settings::HandleGainIndex(const std::string& value)
{
    int16 gainIndex;
    if (!StrToNumber<int16>(value, gainIndex))
        return false;

    return SetGainIndex(gainIndex);
}

bool pm::Settings::HandleClrCycles(const std::string& value)
{
    uns16 clrCycles;
    if (!StrToNumber<uns16>(value, clrCycles))
        return false;

    return SetClrCycles(clrCycles);
}

bool pm::Settings::HandleClrMode(const std::string& value)
{
    int32 clrMode;
    if (value == "never")
        clrMode = CLEAR_NEVER;
    else if (value == "pre-exp")
        clrMode = CLEAR_PRE_EXPOSURE;
    else if (value == "pre-seq")
        clrMode = CLEAR_PRE_SEQUENCE;
    else if (value == "post-seq")
        clrMode = CLEAR_POST_SEQUENCE;
    else if (value == "pre-post-seq")
        clrMode = CLEAR_PRE_POST_SEQUENCE;
    else if (value == "pre-exp-post-seq")
        clrMode = CLEAR_PRE_EXPOSURE_POST_SEQ;
    else
        return false;

    return SetClrMode(clrMode);
}

bool pm::Settings::HandleTrigMode(const std::string& value)
{
    int32 trigMode;
    // Classic modes
    if (value == "timed")
        trigMode = TIMED_MODE;
    else if (value == "strobed")
        trigMode = STROBED_MODE;
    else if (value == "bulb")
        trigMode = BULB_MODE;
    else if (value == "trigger-first")
        trigMode = TRIGGER_FIRST_MODE;
    else if (value == "flash")
        trigMode = FLASH_MODE;
    else if (value == "variable-timed")
        trigMode = VARIABLE_TIMED_MODE;
    else if (value == "int-strobe")
        trigMode = INT_STROBE_MODE;
    // Extended modes
    else if (value == "ext-internal")
        trigMode = EXT_TRIG_INTERNAL;
    else if (value == "ext-trig-first")
        trigMode = EXT_TRIG_TRIG_FIRST;
    else if (value == "ext-edge-raising")
        trigMode = EXT_TRIG_EDGE_RISING;
    else
        return false;

    return SetTrigMode(trigMode);
}

bool pm::Settings::HandleExpOutMode(const std::string& value)
{
    int32 expOutMode;
    if (value == "first-row")
        expOutMode = EXPOSE_OUT_FIRST_ROW;
    else if (value == "all-rows")
        expOutMode = EXPOSE_OUT_ALL_ROWS;
    else if (value == "any-row")
        expOutMode = EXPOSE_OUT_ANY_ROW;
    else if (value == "rolling-shutter")
        expOutMode = EXPOSE_OUT_ROLLING_SHUTTER;
    else
        return false;

    return SetExpOutMode(expOutMode);
}

bool pm::Settings::HandleMetadataEnabled(const std::string& value)
{
    if (!value.empty())
        return false;
    m_metadataEnabled = true;

    return true;
}

bool pm::Settings::HandleTrigTabSignal(const std::string& value)
{
    int32 trigTabSignal;
    if (value == "expose-out")
        trigTabSignal = PL_TRIGTAB_SIGNAL_EXPOSE_OUT;
    else
        return false;

    return SetTrigTabSignal(trigTabSignal);
}

bool pm::Settings::HandleLastMuxedSignal(const std::string& value)
{
    uns8 lastMuxedSignal;
    if (!StrToNumber<uns8>(value, lastMuxedSignal))
        return false;

    return SetLastMuxedSignal(lastMuxedSignal);
}

bool pm::Settings::HandleAcqFrameCount(const std::string& value)
{
    uns32 acqFrameCount;
    if (!StrToNumber<uns32>(value, acqFrameCount))
        return false;

    return SetAcqFrameCount(acqFrameCount);
}

bool pm::Settings::HandleBufferFrameCount(const std::string& value)
{
    uns32 bufferFrameCount;
    if (!StrToNumber<uns32>(value, bufferFrameCount))
        return false;

    return SetBufferFrameCount(bufferFrameCount);
}

bool pm::Settings::HandleBinningSerial(const std::string& value)
{
    uns16 binSer;
    if (!StrToNumber<uns16>(value, binSer))
        return false;

    return SetBinningSerial(binSer);
}

bool pm::Settings::HandleBinningParallel(const std::string& value)
{
    uns16 binPar;
    if (!StrToNumber<uns16>(value, binPar))
        return false;

    return SetBinningParallel(binPar);
}

bool pm::Settings::HandleRegions(const std::string& value)
{
    std::vector<rgn_type> regions;

    if (value.empty())
    {
        // No regions means full sensor size will be used
        return SetRegions(regions);
    }

    const std::vector<std::string> rois =
        SplitString(value, Option::ValueGroupsSeparator);

    if (rois.size() == 0)
    {
        Log::LogE("Incorrect number of values for ROI");
        return false;
    }

    for (std::string roi : rois)
    {
        rgn_type rgn;
        rgn.sbin = m_binSer;
        rgn.pbin = m_binPar;

        const std::vector<std::string> values =
            SplitString(roi, Option::ValuesSeparator);

        if (values.size() != 4)
        {
            Log::LogE("Incorrect number of values for ROI");
            return false;
        }

        if (!StrToNumber<uns16>(values[0], rgn.s1)
                || !StrToNumber<uns16>(values[1], rgn.s2)
                || !StrToNumber<uns16>(values[2], rgn.p1)
                || !StrToNumber<uns16>(values[3], rgn.p2))
        {
            Log::LogE("Incorrect value(s) for ROI");
            return false;
        }

        regions.push_back(rgn);
    }

    return SetRegions(regions);
}

bool pm::Settings::HandleExposure(const std::string& value)
{
    const size_t suffixPos = value.find_first_not_of("0123456789");

    std::string rawValue(value);
    std::string suffix;
    if (suffixPos != std::string::npos)
    {
        suffix = value.substr(suffixPos);
        // Remove the suffix from value
        rawValue = value.substr(0, suffixPos);
    }

    uns32 expTime;
    if (!StrToNumber<uns32>(rawValue, expTime))
        return false;

    int32 expTimeRes;
    if (suffix == "us")
        expTimeRes = EXP_RES_ONE_MICROSEC;
    else if (suffix == "ms" || suffix.empty()) // ms is default resolution
        expTimeRes = EXP_RES_ONE_MILLISEC;
    else if (suffix == "s")
        expTimeRes = EXP_RES_ONE_SEC;
    else
        return false;

    if (!SetExposure(expTime))
        return false;
    return SetExposureResolution(expTimeRes);
}

bool pm::Settings::HandleAcqMode(const std::string& value)
{
    AcqMode acqMode;
    if (value == "snap-seq")
        acqMode = AcqMode::SnapSequence;
    else if (value == "snap-circ-buffer")
        acqMode = AcqMode::SnapCircBuffer;
    else if (value == "snap-time-lapse")
        acqMode = AcqMode::SnapTimeLapse;
    else if (value == "live-circ-buffer")
        acqMode = AcqMode::LiveCircBuffer;
    else if (value == "live-time-lapse")
        acqMode = AcqMode::LiveTimeLapse;
    else
        return false;

    return SetAcqMode(acqMode);
}

bool pm::Settings::HandleTimeLapseDelay(const std::string& value)
{
    unsigned int timeLapseDelay;
    if (!StrToNumber<unsigned int>(value, timeLapseDelay))
        return false;

    return SetTimeLapseDelay(timeLapseDelay);
}

bool pm::Settings::HandleStorageType(const std::string& value)
{
    StorageType storageType;
    if (value == "none")
        storageType = StorageType::None;
    else if (value == "prd")
        storageType = StorageType::Prd;
    else if (value == "tiff")
        storageType = StorageType::Tiff;
    else
        return false;

    return SetStorageType(storageType);
}

bool pm::Settings::HandleSaveDir(const std::string& value)
{
    return SetSaveDir(value);
}

bool pm::Settings::HandleSaveFirst(const std::string& value)
{
    size_t saveFirst;
    if (!StrToNumber<size_t>(value, saveFirst))
        return false;

    return SetSaveFirst(saveFirst);
}

bool pm::Settings::HandleSaveLast(const std::string& value)
{
    size_t saveLast;
    if (!StrToNumber<size_t>(value, saveLast))
        return false;

    return SetSaveLast(saveLast);
}

bool pm::Settings::HandleMaxStackSize(const std::string& value)
{
    const size_t suffixPos = value.find_last_of("kMG");
    char suffix = ' ';
    std::string rawValue(value);
    if (suffixPos != std::string::npos && suffixPos == value.length() - 1)
    {
        suffix = value[suffixPos];
        // Remove the suffix from value
        rawValue.pop_back();
    }

    size_t bytes;
    if (!StrToNumber<size_t>(rawValue, bytes))
        return false;
    size_t maxStackSize = bytes;

    const unsigned int maxBits = sizeof(size_t) * 8;

    unsigned int shiftBits = 0;
    if (suffix == 'G')
        shiftBits = 30;
    else if (suffix == 'M')
        shiftBits = 20;
    else if (suffix == 'k')
        shiftBits = 10;

    unsigned int currentBits = 1; // At least one bit
    while ((bytes >> currentBits) > 0)
        currentBits++;

    if (currentBits + shiftBits > maxBits)
    {
        Log::LogE("Value '%s' is too big (maxBits: %u, currentBits: %u, "
                "shiftBits: %u", value.c_str(), maxBits, currentBits, shiftBits);
        return false;
    }

    maxStackSize <<= shiftBits;

    return SetMaxStackSize(maxStackSize);
}

bool pm::Settings::HandleCentroidsEnabled(const std::string& value)
{
    if (!value.empty())
        return false;
    m_centroidsEnabled = true;

    return true;
}

bool pm::Settings::HandleCentroidCount(const std::string& value)
{
    uns16 count;
    if (!StrToNumber<uns16>(value, count))
        return false;

    return SetCentroidCount(count);
}

bool pm::Settings::HandleCentroidRadius(const std::string& value)
{
    uns16 radius;
    if (!StrToNumber<uns16>(value, radius))
        return false;

    return SetCentroidRadius(radius);
}
