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

    if (RY3_SUCCESS != WzRenderThread::getHandle(count, &handle))
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

    if (RY3_SUCCESS != WzRenderThread::getHandle(count, &handle))
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

QVariantMap WzSetting2::readAdminSettingDefault()
{
    auto buf = AdminSettingStruct();
    return buf2Map(buf);
}

int WzSetting2::readAdminSetting2(QVariantMap &params)
{
#ifndef HARDLOCK
    return SHST_NO_HARDLOCK;
#endif
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
        return RY3_NOT_FOUND;

    RY_HANDLE handle;

    if (RY3_SUCCESS != WzRenderThread::getHandle(count, &handle))
        return SHST_RY3_OPEN_FAILED;

    AdminSettingStruct buf;
    unsigned long ret = RY3_Read(handle, 0, reinterpret_cast<unsigned char*>(&buf), sizeof(AdminSettingStruct));
    // 化学发光曝光时间是0就代表加密锁中从没有保存过数据, 正常情况下不应该是0的曝光时间
    if (ret != RY3_SUCCESS || buf.ChemiExposureMs == 0) {
        return SHST_RY3_READ_FAILED;
        //buf = AdminSettingStruct();
    }

    RY3_Close(handle, true);

    params = buf2Map(buf);
    params["RY3ID"] = WzRenderThread::sn;

    return SHST_SUCCESS;
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
    params["repeatSetFilter"] = getBit(buf.checked1, 2);
    params["pvcamSlowPreview"] = getBit(buf.checked1, 3);
    params["isFilterWheel8"] = getBit(buf.checked1, 4);
    params["customFilterWheel"] = getBit(buf.checked1, 5);
    params["bluePenetrateAlone"] = getBit(buf.checked1, 6);
    params["hideUvPenetrate"] = getBit(buf.checked1, 7);
    params["hideUvPenetrateForce"] = getBit(buf.checked2, 0);
    params["fluorPreviewExposureMs"] = buf.FluorPreviewExposureMs;

    QVariantList filterOptions;
    for (int i = 0; i < 8; i++) {
        QVariantMap filter;
        int offset = i * 5;
        filter["color"] = QColor(qRgb(buf.FilterOptions[offset + 0],
                                      buf.FilterOptions[offset + 1],
                                      buf.FilterOptions[offset + 2])).name();
        filter["wavelength"] = buf.FilterOptions[offset + 3] << 8 |
                               buf.FilterOptions[offset + 4];
        filterOptions.append(filter);
    }
    params["filters"] = filterOptions;

    QVariantList layerPWMCount;
    for (int i = 0; i < 8; i++) {
        layerPWMCount.append(buf.layerPWMCount[i]);
    }
    params["layerPWMCount"] = layerPWMCount;

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
    setBit(buf.checked1, 2, params["repeatSetFilter"].toBool());
    setBit(buf.checked1, 3, params["pvcamSlowPreview"].toBool());
    setBit(buf.checked1, 4, params["isFilterWheel8"].toBool());
    setBit(buf.checked1, 5, params["customFilterWheel"].toBool());
    setBit(buf.checked1, 6, params["bluePenetrateAlone"].toBool());
    setBit(buf.checked1, 7, params["hideUvPenetrate"].toBool());
    setBit(buf.checked2, 0, params["hideUvPenetrateForce"].toBool());

    buf.FluorPreviewExposureMs = params["fluorPreviewExposureMs"].toInt();

    QVariantList filterOptions = params["filters"].toList();
    for (int i = 0; i < filterOptions.length(); i++) {
        if (i >= 8)
            break;

        QVariantMap filter = filterOptions.at(i).toMap();
        int offset = i * 5;

        QColor color(filter["color"].toString());
        buf.FilterOptions[offset + 0] = color.red() & 0xff;
        buf.FilterOptions[offset + 1] = color.green() & 0xff;
        buf.FilterOptions[offset + 2] = color.blue() & 0xff;

        int wavelength = filter["wavelength"].toInt();
        buf.FilterOptions[offset + 3] = (wavelength & 0xff00) >> 8;
        buf.FilterOptions[offset + 4] = wavelength & 0xff;
    }

    QVariantList layerPWMCount = params["layerPWMCount"].toList();
    for (int i = 0; i < layerPWMCount.length(); i++) {
        if (i >= 8)
            break;

        buf.layerPWMCount[i] = layerPWMCount.at(i).toInt();
    }
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
