#ifndef GLOBALENUM_H
#define GLOBALENUM_H

#include <QObject>

class WzEnum : public QObject
{
    Q_OBJECT
public:
    WzEnum(QObject *parent=nullptr): QObject(parent) {}
    enum ImageSource{Capture,
                     Open};
    Q_ENUM(ImageSource)

    enum ErrorCode {
        Success,
        FileNotFound,
        CannotOpen,
        NullBuffer,
        UnsupportedFormat
    };
    Q_ENUM(ErrorCode)

    enum RGBChannel {
        Null,
        Red,
        Green,
        Blue
    };
    Q_ENUM(RGBChannel)

    enum LightType {
        Light_None,
        Light_WhiteUp,
        Light_UvReflex,
        Light_WhiteDown,
        Light_UvPenetrate,
        Light_Red,
        Light_Green,
        Light_Blue,
        Light_BluePenetrate,
        Light_UvReflex1,
        Light_UvReflex2,        
        Light_First = Light_None,
        Light_Last = Light_UvReflex2
    };
    Q_ENUM(LightType)

    enum CaptureMode {
        CaptureMode_Chemi,
        CaptureMode_RNA_DNA,
        CaptureMode_Protein,
        CaptureMode_Fluor
    };
    Q_ENUM(CaptureMode)
};


#endif // GLOBALENUM_H
