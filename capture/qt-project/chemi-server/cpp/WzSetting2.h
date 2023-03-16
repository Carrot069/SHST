#ifndef WZSETTING2_H
#define WZSETTING2_H

#include <QObject>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef HARDLOCK
#include "RY3_API.h"
#endif

struct AdminSettingStruct {
    uint8_t ChemiLight = 1;
    uint8_t ChemiFilter = 0;
    uint8_t ChemiAperture = 6;
    uint8_t ChemiBinning = 3;
    uint32_t ChemiExposureMs = 5000;
    uint32_t ChemiPreviewExposureMs = 10;

    uint8_t RNALight = 4;
    uint8_t RNAFilter = 3;
    uint8_t RNAAperture = 6;
    uint8_t RNABinning = 1;
    uint32_t RNAExposureMs = 50;

    uint8_t ProteinLight = 3;
    uint8_t ProteinFilter = 0;
    uint8_t ProteinAperture = 0;
    uint8_t ProteinBinning = 1;
    uint32_t ProteinExposureMs = 1;

    uint8_t RedLight = 5;
    uint8_t RedFilter = 1;
    uint8_t RedAperture = 6;
    uint8_t RedBinning = 3;
    uint32_t RedExposureMs = 10;

    uint8_t GreenLight = 6;
    uint8_t GreenFilter = 2;
    uint8_t GreenAperture = 6;
    uint8_t GreenBinning = 3;
    uint32_t GreenExposureMs = 10;

    uint8_t BlueLight = 7;
    uint8_t BlueFilter = 4;
    uint8_t BlueAperture = 6;
    uint8_t BlueBinning = 3;
    uint32_t BlueExposureMs = 10;

    uint8_t checked1 = 0;

    uint16_t FluorPreviewExposureMs = 10;
};

class WzSetting2
{
public:
    WzSetting2();

    static void saveAdminSetting(QVariantMap params);
    static QVariantMap readAdminSetting();

    static QVariantMap buf2Map(AdminSettingStruct &buf);
    static void map2Buf(QVariantMap &params, AdminSettingStruct &buf);

    static void setBit(uint8_t &i, const uint8_t pos, const bool value);
    static bool getBit(uint8_t &i, const uint8_t pos);
};

#endif // WZSETTING2_H
