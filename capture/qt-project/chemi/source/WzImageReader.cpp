#include "WzImageReader.h"

WzTiffCloser::WzTiffCloser(TIFF* tif) {
    g_tif = tif;
}

WzTiffCloser::~WzTiffCloser() {
    TIFFClose(g_tif);
}

WzImageReader::WzImageReader(QObject *parent) : QObject(parent)
{

}

WzEnum::ErrorCode WzImageReader::loadImage(const QString &imageFileName, WzImageBuffer *imageBuffer)
{
    QUrl url(imageFileName);
    if (!QFile::exists(imageFileName) && !QFile::exists(url.toLocalFile()))
        return WzEnum::FileNotFound;

#ifdef Q_OS_WIN
    wchar_t fn[65535] = {0};

    if (url.toLocalFile() == "")
        imageFileName.toWCharArray(fn);
    else
        url.toLocalFile().toWCharArray(fn);

    TIFF* tif = TIFFOpenW(fn, "r");
#else
    QByteArray fnByteArray;
    if (url.toLocalFile() == "")
        fnByteArray = imageFileName.toLocal8Bit();
    else
        fnByteArray = url.toLocalFile().toLocal8Bit();
    char* fn =  fnByteArray.data();

    TIFF* tif = TIFFOpen(fn, "r");
#endif
    if (!tif)
        return WzEnum::CannotOpen;

    WzTiffCloser closer(tif);
    tstrip_t stripCount = TIFFNumberOfStrips(tif);
    tsize_t stripSize = TIFFStripSize(tif);
    unsigned char* bufPtr;
    tstrip_t stripIndex;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageBuffer->width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageBuffer->height);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &imageBuffer->bitDepth);

    if (imageBuffer->bitDepth != 16) {
        return WzEnum::UnsupportedFormat;
    }

    // TODO 尝试读取曝光时间

    if (imageBuffer->buf) {
        delete []imageBuffer->buf;
        imageBuffer->buf = nullptr;
    }
    imageBuffer->bytesCountOfBuf = static_cast<uint32_t>(stripCount) * static_cast<uint32_t>(stripSize);
    imageBuffer->buf = new uint8[imageBuffer->bytesCountOfBuf];
    bufPtr = static_cast<unsigned char*>(imageBuffer->buf);
    for (stripIndex=0; stripIndex<stripCount; stripIndex++) {
        TIFFReadEncodedStrip(tif, stripIndex, bufPtr, static_cast<tsize_t>(stripSize));
        bufPtr += stripSize;
    }

    imageBuffer->update();

    return WzEnum::Success;
}
