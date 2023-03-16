#include "WzSetting2.h"

WzSetting2::WzSetting2()
{

}

void WzSetting2::saveAdminSetting(QVariantMap params)
{
#ifdef HARDLOCK
    unsigned char v1 = 0x34;
    unsigned char v2 = 0x32;
    unsigned char v3 = 0x38;
    unsigned char v4 = 0x30;
    unsigned char v5 = 0x35;
    unsigned char v6 = 0x39;
    unsigned char v7 = 0x32;
    unsigned char v8 = 0x38;

    char vid[9];
    vid[8] = 0;

    for (int i = 0; i < 9; i++) {
        switch (i) {
        case 0: vid[i] = v1; break;
        case 1: vid[i] = v2; break;
        case 2: vid[i] = v3; break;
        case 3: vid[i] = v4; break;
        case 4: vid[i] = v5; break;
        case 5: vid[i] = v6; break;
        case 6: vid[i] = v7; break;
        case 7: vid[i] = v8; break;
        case 8: vid[i] = 0; break;
        }
    }

    int count = 0;
    RY3_Find(vid, &count);

    if (count == 0)
        return;

    RY_HANDLE handle;

    if (RY3_SUCCESS != RY3_Open(&handle, 1))
        return;

    AdminSettingStruct buf;
    map2Buf(params, buf);

    RY3_Write(handle, 0, reinterpret_cast<unsigned char*>(&buf), sizeof(AdminSettingStruct));

    RY3_Close(handle, true);
#else
    Q_UNUSED(params)
#endif
}

QVariantMap WzSetting2::readAdminSetting()
{
#ifdef HARDLOCK
    unsigned char v1 = 0x34;
    unsigned char v2 = 0x32;
    unsigned char v3 = 0x38;
    unsigned char v4 = 0x30;
    unsigned char v5 = 0x35;
    unsigned char v6 = 0x39;
    unsigned char v7 = 0x32;
    unsigned char v8 = 0x38;

    char vid[9];
    vid[8] = 0;

    for (int i = 0; i < 9; i++) {
        switch (i) {
        case 0: vid[i] = v1; break;
        case 1: vid[i] = v2; break;
        case 2: vid[i] = v3; break;
        case 3: vid[i] = v4; break;
        case 4: vid[i] = v5; break;
        case 5: vid[i] = v6; break;
        case 6: vid[i] = v7; break;
        case 7: vid[i] = v8; break;
        case 8: vid[i] = 0; break;
        }
    }

    int count = 0;
    RY3_Find(vid, &count);

    if (count == 0)
        return QVariantMap();

    RY_HANDLE handle;

    if (RY3_SUCCESS != RY3_Open(&handle, 1))
        return QVariantMap();

    AdminSettingStruct buf;
    unsigned long ret = RY3_Read(handle, 0, reinterpret_cast<unsigned char*>(&buf), sizeof(AdminSettingStruct));
    // 化学发光曝光时间是0就代表加密锁中从没有保存过数据, 正常情况下不应该是0的曝光时间, 所以在这里读取默认值
    if (ret != RY3_SUCCESS || buf.ChemiExposureMs == 0) {
        buf = AdminSettingStruct();
    }

    RY3_Close(handle, true);

    return buf2Map(buf);

#else
    return QVariantMap();
#endif
}

QVariantMap WzSetting2::buf2Map(AdminSettingStruct &buf)
{
    QVariantMap params;

    QVariantMap chemi;
    chemi["light"]              = buf.ChemiLight;
    chemi["filter"]             = buf.ChemiFilter;
    chemi["aperture"]           = buf.ChemiAperture;
    chemi["binning"]            = buf.ChemiBinning;
    chemi["exposureMs"]         = buf.ChemiExposureMs;
    chemi["previewExposureMs"]  = buf.ChemiPreviewExposureMs;
    params["chemi"] = chemi;

    QVariantMap rna;
    rna["light"     ] = buf.RNALight     ;
    rna["filter"    ] = buf.RNAFilter    ;
    rna["aperture"  ] = buf.RNAAperture  ;
    rna["binning"   ] = buf.RNABinning   ;
    rna["exposureMs"] = buf.RNAExposureMs;
    params["rna"] = rna;

    QVariantMap protein;
    protein["light"     ] = buf.ProteinLight     ;
    protein["filter"    ] = buf.ProteinFilter    ;
    protein["aperture"  ] = buf.ProteinAperture  ;
    protein["binning"   ] = buf.ProteinBinning   ;
    protein["exposureMs"] = buf.ProteinExposureMs;
    params["protein"] = protein;

    QVariantMap red;
    red["light"     ] = buf.RedLight     ;
    red["filter"    ] = buf.RedFilter    ;
    red["aperture"  ] = buf.RedAperture  ;
    red["binning"   ] = buf.RedBinning   ;
    red["exposureMs"] = buf.RedExposureMs;
    params["red"] = red;

    QVariantMap green;
    green["light"     ] = buf.GreenLight     ;
    green["filter"    ] = buf.GreenFilter    ;
    green["aperture"  ] = buf.GreenAperture  ;
    green["binning"   ] = buf.GreenBinning   ;
    green["exposureMs"] = buf.GreenExposureMs;
    params["green"] = green;

    QVariantMap blue;
    blue["light"     ] = buf.BlueLight     ;
    blue["filter"    ] = buf.BlueFilter    ;
    blue["aperture"  ] = buf.BlueAperture  ;
    blue["binning"   ] = buf.BlueBinning   ;
    blue["exposureMs"] = buf.BlueExposureMs;
    params["blue"] = blue;

    params["binningVisible"] = (buf.checked1 & 0x01) == 0x00 ? true : false;
    params["grayAccumulateAddExposure"] = getBit(buf.checked1, 1);

    return params;
}

void WzSetting2::map2Buf(QVariantMap &params, AdminSettingStruct &buf)
{
    buf.ChemiLight              = params["chemi"].toMap()["light"].toInt();
    buf.ChemiFilter             = params["chemi"].toMap()["filter"].toInt();
    buf.ChemiAperture           = params["chemi"].toMap()["aperture"].toInt();
    buf.ChemiBinning            = params["chemi"].toMap()["binning"].toInt();
    buf.ChemiExposureMs         = params["chemi"].toMap()["exposureMs"].toInt();
    buf.ChemiPreviewExposureMs  = params["chemi"].toMap()["previewExposureMs"].toInt();

    buf.RNALight      = params["rna"].toMap()["light"].toInt();
    buf.RNAFilter     = params["rna"].toMap()["filter"].toInt();
    buf.RNAAperture   = params["rna"].toMap()["aperture"].toInt();
    buf.RNABinning    = params["rna"].toMap()["binning"].toInt();
    buf.RNAExposureMs = params["rna"].toMap()["exposureMs"].toInt();

    buf.ProteinLight      = params["protein"].toMap()["light"].toInt();
    buf.ProteinFilter     = params["protein"].toMap()["filter"].toInt();
    buf.ProteinAperture   = params["protein"].toMap()["aperture"].toInt();
    buf.ProteinBinning    = params["protein"].toMap()["binning"].toInt();
    buf.ProteinExposureMs = params["protein"].toMap()["exposureMs"].toInt();

    buf.RedLight      = params["red"].toMap()["light"].toInt();
    buf.RedFilter     = params["red"].toMap()["filter"].toInt();
    buf.RedAperture   = params["red"].toMap()["aperture"].toInt();
    buf.RedBinning    = params["red"].toMap()["binning"].toInt();
    buf.RedExposureMs = params["red"].toMap()["exposureMs"].toInt();

    buf.GreenLight      = params["green"].toMap()["light"].toInt();
    buf.GreenFilter     = params["green"].toMap()["filter"].toInt();
    buf.GreenAperture   = params["green"].toMap()["aperture"].toInt();
    buf.GreenBinning    = params["green"].toMap()["binning"].toInt();
    buf.GreenExposureMs = params["green"].toMap()["exposureMs"].toInt();

    buf.BlueLight      = params["blue"].toMap()["light"].toInt();
    buf.BlueFilter     = params["blue"].toMap()["filter"].toInt();
    buf.BlueAperture   = params["blue"].toMap()["aperture"].toInt();
    buf.BlueBinning    = params["blue"].toMap()["binning"].toInt();
    buf.BlueExposureMs = params["blue"].toMap()["exposureMs"].toInt();

    setBit(buf.checked1, 0, !params["binningVisible"].toBool());
    setBit(buf.checked1, 1, params["grayAccumulateAddExposure"].toBool());
}

void WzSetting2::setBit(uint8_t &i, const uint8_t pos, const bool value)
{
    if (value)
        i = i | (1 << pos);
    else {
        uint8_t j = 1 << pos;
        i = i & (!j);
    }
}

bool WzSetting2::getBit(uint8_t &i, const uint8_t pos)
{
    return (1 << pos) & i;
}
