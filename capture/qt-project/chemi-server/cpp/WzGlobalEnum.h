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
    Q_ENUMS(RGBChannel)
};


#endif // GLOBALENUM_H
