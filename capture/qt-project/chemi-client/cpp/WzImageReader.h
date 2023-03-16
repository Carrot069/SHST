#ifndef WZIMAGEREADER_H
#define WZIMAGEREADER_H

#include <QObject>
#include <QUrl>
#include <QFile>

#include "tiff.h"
#include "tiffio.h"

#include "WzGlobalEnum.h"
#include "WzImageBuffer.h"

class WzTiffCloser {
private:
    TIFF* g_tif;
public:
    WzTiffCloser(TIFF* tif);
    ~WzTiffCloser();
};

class WzImageReader : public QObject
{
    Q_OBJECT
public:
    explicit WzImageReader(QObject *parent = nullptr);

    WzEnum::ErrorCode loadImage(const QString& imageFileName, WzImageBuffer* imageBuffer);
signals:

};

#endif // WZIMAGEREADER_H
