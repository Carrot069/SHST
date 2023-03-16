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

    enum AnalysisAction {
        AnalysisActionNone,
        ManualAddArea,
        ManualAddLane,
        ManualAddBand,
        RemoveArea,
        RemoveLane,
        RemoveBand,
        AutoLane,
        AutoBand,
        SelectBackground,
        HorizontalRotate,
        CropImage,
        AddRectangle,
        AddEllipse,
        AddArrow,
        AddText,
        DeleteAnnotation
    };
    Q_ENUM(AnalysisAction);
};


#endif // GLOBALENUM_H
