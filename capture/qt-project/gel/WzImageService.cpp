#include "WzImageService.h"

WzImageService* g_imageService = nullptr;

WzTiffCloser::WzTiffCloser(TIFF* tif) {
    g_tif = tif;
}

WzTiffCloser::~WzTiffCloser() {
    TIFFClose(g_tif);
}

void WzImageServiceThread::run() {

}

void WzImageServiceThread::addAction(Action action) {
    m_actions.append(action);
}

void WzImageServiceThread::openImage(const QString& filename, WzImageBuffer* imageBuffer) {
    Q_UNUSED(filename)
    Q_UNUSED(imageBuffer)
}

WzImageService::WzImageService(QObject *parent) :
    QObject(parent),
    m_imageBuffer(new WzImageBuffer)
{
    g_imageService = this;
    qDebug() << "WzImageService created";

    scanPalettePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + kPaletteFolder);
}

WzImageService::~WzImageService() {
    if (m_imageBuffer->buf) {
        delete []m_imageBuffer->buf;
        m_imageBuffer->buf = nullptr;
    }
    delete m_imageBuffer;
    if (nullptr != m_markerImage) {
        delete m_markerImage;
        m_markerImage = nullptr;
    }
    if (nullptr != m_image32bit) {
        delete []m_image32bit;
        m_image32bit = nullptr;
    }
    if (nullptr != m_image8bit) {
        delete []m_image8bit;
        m_image8bit = nullptr;
    }
    if (nullptr != m_printImage) {
        delete m_printImage;
        m_printImage = nullptr;
    }
}

// TODO 考虑改成异步增加界面响应速度
void WzImageService::openImage(const QString& fileName) {
    QString localFile = "";
    QUrl fileUrl(fileName);
    if (fileUrl.isLocalFile())
        localFile = fileUrl.toLocalFile();
    else
        localFile = fileName;
    if (nullptr != m_imageBuffer && fileName == m_imageBuffer->imageFile)
        return;
    WzEnum::ErrorCode ret = loadImage(localFile, m_imageBuffer);
    if (ret != WzEnum::Success) {
        emit imageOpened(ret);
        return;
    }
    m_imageBuffer->imageFile = localFile;
    emit imageOpened(ret);
}

WzEnum::ErrorCode WzImageService::loadImage(const QString& imageFileName, WzImageBuffer* imageBuffer) {
    if (!imageBuffer)
        return WzEnum::NullBuffer;

    QUrl url(imageFileName);
    if (!QFile::exists(imageFileName) && !QFile::exists(url.toLocalFile()))
        return WzEnum::FileNotFound;

#ifdef Q_OS_WIN
    wchar_t fn[65535] = {0};
    imageFileName.toWCharArray(fn);

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
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &imageBuffer->samplesPerPixel);

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
    if (imageBuffer->bitDepth == 8 && imageBuffer->samplesPerPixel == 3)
        imageBuffer->imageThumbFile = createThumbRGB(*imageBuffer, imageFileName);
    else
        imageBuffer->imageThumbFile = createThumb(*imageBuffer, imageFileName);

    return WzEnum::Success;
}

bool WzImageService::saveImageAsTiff(const WzImageBuffer& imageBuffer, const QString& filename) {
#ifdef Q_OS_WIN
    wchar_t fn[65535] = {0};
    filename.toWCharArray(fn);

    TIFF* tif = TIFFOpenW(fn, "w");
#else
    QByteArray filenameByteArray = filename.toLocal8Bit();
    char* fn = filenameByteArray.data();

    TIFF* tif = TIFFOpen(fn, "w");
#endif

    if (!tif)
        return false;

    {
        WzTiffCloser closer(tif);

        TIFFSetField(tif, TIFFTAG_SUBFILETYPE, 0);
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, imageBuffer.width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, imageBuffer.height);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, imageBuffer.bitDepth);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, ""); // TODO 写点东西
        TIFFSetField(tif, TIFFTAG_MAKE, kTiffMake);
        TIFFSetField(tif, TIFFTAG_MODEL, ""); // TODO 写点东西， 本程序拍摄的图片写入这个字段
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, imageBuffer.samplesPerPixel);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
        TIFFSetField(tif, TIFFTAG_XRESOLUTION, 600.0);
        TIFFSetField(tif, TIFFTAG_YRESOLUTION, 600.0);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
        TIFFSetField(tif, TIFFTAG_SOFTWARE, kTiffSoftware);
        TIFFSetField(tif, TIFFTAG_ARTIST, kTiffArtist);
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
        // TODO 找出图片的最大最小灰阶
        //TIFFSetField(tif, TIFFTAG_SMINSAMPLEVALUE,
        //TIFFSetField(tif, TIFFTAG_SMAXSAMPLEVALUE

        uint8_t* pBuf = static_cast<uint8_t*>(imageBuffer.buf);
        for (uint32_t i = 0; i < imageBuffer.height; i++) {
            TIFFWriteRawStrip(tif, i, pBuf, imageBuffer.width * (imageBuffer.bitDepth / 8) * imageBuffer.samplesPerPixel);
            pBuf += imageBuffer.width * (imageBuffer.bitDepth / 8) * imageBuffer.samplesPerPixel;
        }

        qInfo() << "TIFF Saved: " << filename;
    }

#ifdef Q_OS_WIN
    std::wstring fn2(fn);
#else
    std::string fn2(fn);
#endif
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fn2);
    if (image.get()) {
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        exifData["Exif.Image.ExposureTime"] = Exiv2::Rational(imageBuffer.exposureMs, 1000);
        exifData["Exif.Photo.ExposureTime"] = Exiv2::Rational(imageBuffer.exposureMs, 1000);
        QString dateTimeStr = imageBuffer.captureDateTime.toString("yyyy:MM:dd hh:mm:ss");
        if (dateTimeStr != "") {
            exifData["Exif.Image.DateTimeOriginal"] = dateTimeStr.toStdString();
            exifData["Exif.Photo.DateTimeOriginal"] = dateTimeStr.toStdString();
        }
        image->setExifData(exifData);
        image->writeMetadata();
        qInfo() << "exiv2 saved";
    } else {
        qWarning() << "exiv2.open error";
    }

    qInfo("TIFF Saved: %s", fn);

    return true;
};

QString WzImageService::getThumbFileName(const QString& imageFileName) {
    QFileInfo srcFile(imageFileName);
    QDir thumbPath(srcFile.path());
    thumbPath.mkdir(kThumbFolder);
    thumbPath.cd(kThumbFolder);

#ifdef WINNT
    std::wstring path1 = thumbPath.absolutePath().toStdWString();
    SetFileAttributes(path1.data(), FILE_ATTRIBUTE_HIDDEN);
#endif

    QFileInfo thumbFileInfo(thumbPath.path(), srcFile.completeBaseName() + ".jpg");
    return thumbFileInfo.absoluteFilePath();
}

void WzImageService::updateLowHigh(const int& low, const int& high) {
    int l = low;
    int h = high;

    if (l < 0) l = 0;
    if (h > 65535) h = 65535;

    if (l > h) l = h;
    if (l == h) {
        qWarning() << "updateLowHigh, low与high相同";
        for(int i=0; i<65536; i++)
            m_grayTable[i] = 127; // 灰色显示
        return;
    }
    for(int i=0; i<=l; i++) {
        m_grayTable[i] = 0;
    }
    for(int i=h; i<=65535; i++) {
        m_grayTable[i] = 255;
    }
    int j = h - l;
    if (j == 1) {
        qWarning() << "updateLowHigh, low与high只相差了1";
        return;
    }
    double scale = static_cast<double>(j+1) / 256.0;
    for(int i=l; i<=h; i++) {
        double k = ((i-l) / scale);
        if (k < 0)
            k = 0;
        else if (k > 255)
            k = 255;
        m_grayTable[i] = static_cast<unsigned char>(k);
    }
    m_imageBuffer->low = low;
    m_imageBuffer->high = high;
};

// TODO 改为异步
void WzImageService::calculateLowHigh() {
    int low = 0, high = 0;
    if (nullptr == m_imageBuffer || m_imageBuffer->bytesCountOfBuf == 0) {
        qWarning() << "calculateLowHigh, 图片缓冲区无内容";
        emit lowHighCalculated(WzEnum::NullBuffer, 0, 0);
        return;
    }
    m_imageBuffer->update();
    m_imageBuffer->getMinMax(low, high);
    emit lowHighCalculated(WzEnum::Success, low, high);
}

QVariantMap WzImageService::getActiveImageInfo() {
    QVariantMap imageInfo;

    if (nullptr == m_imageBuffer)
        return imageInfo;

    // TODO 字段待补全
    imageInfo["imageFile"] = m_imageBuffer->imageFile;
    imageInfo["imageThumbFile"] = m_imageBuffer->imageThumbFile;
    imageInfo["bitDepth"] = m_imageBuffer->bitDepth;
    imageInfo["samplesPerPixel"] = m_imageBuffer->samplesPerPixel;

    return imageInfo;
};

void WzImageService::deleteImage(QVariantMap imageInfo) {
    // TODO
}

void WzImageService::closeActiveImage() {
    QByteArray image8bit = QByteArray(800*600, 0x7f);
    g_imageView->updateImage(image8bit, QSize(800, 600), m_imageBuffer);
}

bool WzImageService::canSaveAsImage() {
    if (nullptr == m_imageBuffer)
        return false;
    if (m_imageBuffer->width == 0)
        return false;
    if (m_imageBuffer->height == 0)
        return false;
    return true;
}

bool WzImageService::saveAsImage(const QJsonObject &params) {
    QString format = params["format"].toString();
    //bool isSaveMarker = params["isSaveMarker"].toBool();
    QString fileName = params["fileName"].toString();
    if (format == "tiff16") {
        return saveImageAsTiff(*m_imageBuffer, fileName);
    }
    if (nullptr == g_imageView) {
        qWarning("g_imageView == nullptr");
        return false;
    }
    QImage* img = g_imageView->image();
    if (nullptr == img) {
        qWarning("g_imageView->image() == nullptr");
        return false;
    }
    QImage img32bit(img->size(), QImage::Format_ARGB32);
    if (m_isPseudoColorBarInImage && !isRgbImage()) {
        QPainter p;
        p.begin(&img32bit);
        p.drawImage(0, 0, *img);
        p.end();
        img = &img32bit;
        drawColorBar(*img);
    }
    if (format == "jpeg")
        return img->save(fileName, "jpeg", 100);
    else if (format == "png")
        return img->save(fileName, "png", 100);
    else if (format == "tiff8") {
        if (nullptr == m_image8bit)
            return false;
        WzImageBuffer imageBuffer8bit;
        imageBuffer8bit.width = m_imageBuffer->width;
        imageBuffer8bit.height = m_imageBuffer->height;
        imageBuffer8bit.exposureMs = params["exposureMs"].toInt();
        imageBuffer8bit.captureDateTime =
            QDateTime::fromString(params["captureDate"].toString(),
                                  "yyyy-MM-ddTHH:mm:ss.zzz");
        imageBuffer8bit.bitDepth = 8;
        imageBuffer8bit.samplesPerPixel = 1;
        imageBuffer8bit.buf = m_image8bit;
        return saveImageAsTiff(imageBuffer8bit, fileName);
    } else if (format == "tiff24") {
        return saveImageAsTiff24(*img, fileName);
    }

    return false;
}

bool WzImageService::printImage(const QString& language) {
    qInfo("printImage");
    if (g_imageView == nullptr) {
        qInfo("imageView == nullptr");
        return false;
    }
    QImage* img = g_imageView->paintImage();
    if (img == nullptr) {
        qInfo("image == nullptr");
        return false;
    }

    if (nullptr != m_printImage) {
        delete m_printImage;
        m_printImage = nullptr;
    }
    m_printImage = new QImage(*img);

    QPrintPreviewDialog previewDialog;

    connect(&previewDialog, SIGNAL(paintRequested(QPrinter*)), this, SLOT(printPreview(QPrinter*)));

    if (language == "zh") {
        QList<QToolBar*> toolbarList = previewDialog.findChildren<QToolBar*>();
        if(!toolbarList.isEmpty()) {
            QList<QAction*> actions = toolbarList.first()->actions();
            actions.at(0)->setToolTip(tr("放大到最宽"));
            actions.at(1)->setToolTip(tr("整页可见"));
            actions.at(4)->setToolTip(tr("缩小"));
            actions.at(5)->setToolTip(tr("放大"));
            actions.at(7)->setToolTip(tr("竖向"));
            actions.at(8)->setToolTip(tr("横向"));
            actions.at(10)->setToolTip(tr("第一页"));
            actions.at(11)->setToolTip(tr("上一页"));
            actions.at(13)->setToolTip(tr("下一页"));
            actions.at(14)->setToolTip(tr("最后一页"));
            actions.at(16)->setToolTip(tr("每屏显示一页"));
            actions.at(17)->setToolTip(tr("每屏显示两页"));
            actions.at(18)->setToolTip(tr("每屏显示四页"));
            actions.at(20)->setToolTip(tr("页面设置"));
            actions.at(21)->setToolTip(tr("打印"));
        }
    }

    if (language == "zh")
        previewDialog.setWindowTitle(tr("打印预览"));
    previewDialog.setWindowState(Qt::WindowMaximized);

    if (QPrintPreviewDialog::Accepted != previewDialog.exec())
        return false;

    return true;
}

void WzImageService::imageRotate(const double &angle, const bool &blackBackground)
{
    if (nullptr == m_imageBuffer)
        return;
    if (nullptr == m_imageBuffer->buf)
        return;
    imageRotate(*m_imageBuffer, angle, blackBackground);
    updateView();
}

void WzImageService::cropImage(const QRect &rect)
{
    if (m_imageBuffer == nullptr)
        return;
    if (m_imageBuffer->buf == nullptr)
        return;
    if (rect.left() < 0)
        return;
    if (rect.right() >= static_cast<int>(m_imageBuffer->width))
        return;
    if (rect.top() < 0)
        return;
    if (rect.bottom() >= static_cast<int>(m_imageBuffer->height))
        return;

    int newWidth = rect.width() - 1;
    int newHeight = rect.height() - 1;
    int bytesCount = m_imageBuffer->bitDepth / 8;
    uint8_t* newBuf = new uint8_t[newWidth * newHeight * bytesCount];

    for (int row = rect.top(); row < rect.bottom(); row++) {
        uint8_t* src = &m_imageBuffer->buf[rect.left() * bytesCount + row * m_imageBuffer->width * bytesCount];
        uint8_t* dst = &newBuf[(row - rect.top()) * newWidth * bytesCount];
        memcpy(dst, src, newWidth * bytesCount);
    }

    delete[] m_imageBuffer->buf;
    m_imageBuffer->width = newWidth;
    m_imageBuffer->height = newHeight;
    m_imageBuffer->bytesCountOfBuf = newWidth * newHeight * bytesCount;
    m_imageBuffer->buf = newBuf;

    updateView();
}

void WzImageService::resetImage()
{
    if (nullptr == m_imageBuffer)
        return;
    if (m_imageBuffer->imageFile == "")
        return;
    loadImage(m_imageBuffer->imageFile, m_imageBuffer);
    updateView();
}

void WzImageService::updateView() {
    if(!g_imageView)
        return;
    if(!m_imageBuffer->buf)
        return;

    if (m_imageBuffer->bitDepth == 8 && m_imageBuffer->samplesPerPixel == 3) {
        updateViewRGB();
        return;
    }

    QImage markerImage;
    uchar* markerData = nullptr;
    if (m_showMarker && nullptr != m_markerImage) {
        markerImage = m_markerImage->scaled(static_cast<int>(m_imageBuffer->width),
                                            static_cast<int>(m_imageBuffer->height),
                                            Qt::IgnoreAspectRatio);
        markerData = markerImage.bits();
    }

    if (nullptr != m_image8bit) {
        delete []m_image8bit;
        m_image8bit = nullptr;
    }
    m_image8bit = new uint8_t[static_cast<int>(m_imageBuffer->width * m_imageBuffer->height)];
    int pixelCount = m_imageBuffer->width * m_imageBuffer->height;
    m_imageBuffer->update();
    uint16* pWord = m_imageBuffer->bit16Array;

    if (nullptr == markerData) {
        if (m_invert)
            for(int i=0; i<pixelCount; i++) {
                m_image8bit[i] = 255 - static_cast<char>(m_grayTable[*pWord]);
                pWord++;
            }
        else
            for(int i=0; i<pixelCount; i++) {
                m_image8bit[i] = static_cast<char>(m_grayTable[*pWord]);
                pWord++;
            }
    } else {
        if (m_showChemi) {
            for(int i=0; i<pixelCount; i++) {
                int gray = markerData[i] - m_grayTable[*pWord];
                if (gray < 0) gray = 0;
                m_image8bit[i] = static_cast<char>(gray);
                pWord++;
            }
        } else {
            for(int i=0; i<pixelCount; i++) {
                m_image8bit[i] = static_cast<char>(markerData[i]);
                pWord++;
            }
        }
    }

    g_imageView->updateImage(m_image8bit, m_imageBuffer->width, m_imageBuffer->height,
                             m_imageBuffer);
};

void WzImageService::updateCurrentImage(const WzImageBuffer& imageBuffer) {
    // 原有缓冲区有数据且数据大小与新的不同时则先修改内存大小
    if (m_imageBuffer->buf && m_imageBuffer->bytesCountOfBuf != imageBuffer.bytesCountOfBuf) {
        delete []m_imageBuffer->buf;
        m_imageBuffer->buf = new uint8_t[imageBuffer.bytesCountOfBuf];
        m_imageBuffer->bytesCountOfBuf = imageBuffer.bytesCountOfBuf;
    }
    if (m_imageBuffer && m_imageBuffer->buf) {
        memcpy(m_imageBuffer->buf, imageBuffer.buf, imageBuffer.bytesCountOfBuf);
        m_imageBuffer->width = imageBuffer.width;
        m_imageBuffer->height = imageBuffer.height;
        m_imageBuffer->bitDepth = imageBuffer.bitDepth;
    }
    updateView();
}

void WzImageService::createThumb(WzImageBuffer& thumbBuffer,
                 WzImageBuffer& imageBuffer,
                 const uint16_t thumbMaxGrayLimit, const bool isNegative) {

    uint scaleInt;
    uint realThumbWidth, realThumbHeight;

    double scaleX = imageBuffer.width / thumbBuffer.width;
    double scaleY = imageBuffer.height / thumbBuffer.height;
    if (scaleX > scaleY) {
        scaleInt = static_cast<uint>(scaleX);
        realThumbWidth = thumbBuffer.width;
        realThumbHeight = static_cast<uint>((thumbBuffer.width * imageBuffer.height / imageBuffer.width));
    } else {
        scaleInt = static_cast<uint>(scaleY);
        realThumbHeight = thumbBuffer.height;
        realThumbWidth = static_cast<uint>(thumbBuffer.height * imageBuffer.width / imageBuffer.height);
    }

    if (scaleInt == 0) {
        qWarning() << "ImageService::createThumb, scaleInt == 0";
        return;
    }

    thumbBuffer.bytesCountOfBuf = realThumbHeight * realThumbWidth;
    thumbBuffer.width = realThumbWidth;
    thumbBuffer.height = realThumbHeight;

    uint *thumbBuffer16bit = new uint[realThumbWidth * realThumbHeight];

    for (uint thumbRow = 0; thumbRow < realThumbHeight; thumbRow++) {
        for (uint thumbCol = 0; thumbCol < realThumbWidth; thumbCol++) {
            uint srcReadyCount = static_cast<uint>((thumbRow * imageBuffer.width * scaleInt) +
                (thumbCol * scaleInt));
            uint grayAccumulate = 0;
            for (uint srcMergeRectY = 0; srcMergeRectY < scaleInt; srcMergeRectY++) {
                for (uint srcMergeRectX = 0; srcMergeRectX < scaleInt; srcMergeRectX++) {
                    const uint16_t* srcImagePtrOffset = imageBuffer.bit16Array;
                    //qDebug() << "thumbRow" << thumbRow << ", thumbCol:" << thumbCol;
                    srcImagePtrOffset += srcReadyCount + srcMergeRectY * imageBuffer.width +
                        srcMergeRectX;
                    grayAccumulate = grayAccumulate + *srcImagePtrOffset;
                    //*thumbBuffer16bitPtr = *thumbBuffer16bitPtr + *srcImagePtrOffset;
                }
            }
            //*thumbBuffer16bitPtr = *thumbBuffer16bitPtr / (scaleInt * scaleInt);
            thumbBuffer16bit[thumbRow * realThumbWidth + thumbCol] = grayAccumulate / (scaleInt * scaleInt);
            //thumbBuffer16bitPtr++;
        }
    }

    // 找最大，最小灰度值
    uint maxGray = thumbBuffer16bit[0];
    uint minGray = thumbBuffer16bit[0];
    for (int i = 0; i < realThumbHeight * realThumbWidth; i++) {
        maxGray = std::max(maxGray, thumbBuffer16bit[i]);
        minGray = std::min(minGray, thumbBuffer16bit[i]);
    }
    maxGray = std::max<uint>(thumbMaxGrayLimit, maxGray);

    // 16位灰阶转8位
    if (maxGray > minGray) {
        double grayScale = static_cast<double>(255) / static_cast<double>((maxGray - minGray));
        thumbBuffer.bitDepth = 8;
        thumbBuffer.buf = new uint8_t[thumbBuffer.bytesCountOfBuf];
        thumbBuffer.update();
        uint8_t* thumbBufferPtr = thumbBuffer.bit8Array;
        for (uint i = 0; i < realThumbHeight * realThumbWidth; i++) {
            if (isNegative)
                *thumbBufferPtr = 255 - static_cast<unsigned char>((thumbBuffer16bit[i] - minGray) * grayScale);
            else
                *thumbBufferPtr = static_cast<unsigned char>((thumbBuffer16bit[i] - minGray) * grayScale);
            thumbBufferPtr++;
        }
    } else {
        thumbBuffer.bitDepth = 8;
        thumbBuffer.buf = new uint8_t[thumbBuffer.bytesCountOfBuf];
        thumbBuffer.update();
        uint8_t* thumbBufferPtr = thumbBuffer.bit8Array;
        for (uint i = 0; i < realThumbHeight * realThumbWidth; i++) {
            if (isNegative)
                *thumbBufferPtr = 0;
            else
                *thumbBufferPtr = 255;
            thumbBufferPtr++;
        }
    }
    delete [] thumbBuffer16bit;
}

void WzImageService::createThumbRGB(QImage& thumb, WzImageBuffer &imageBuffer)
{
    if (imageBuffer.bitDepth != 8 || imageBuffer.samplesPerPixel != 3) {
        qInfo() << "createThumbRGB, bitDepth or samplesPerPixel incorrect";
        return;
    }
    qInfo() << "createThumbRGB:\n\t" <<
               "image.width:" << imageBuffer.width << "\n\t" <<
               "image.height:" << imageBuffer.height << "\n\t" <<
               "image.bytesCountOfBuf:" << imageBuffer.bytesCountOfBuf << "\n";

    QImage bigImage(imageBuffer.width, imageBuffer.height, QImage::Format_RGB888);
    bigImage.fill(Qt::gray);
#ifdef Q_OS_MAC
    memcpy(bigImage.bits(), imageBuffer.buf, imageBuffer.bytesCountOfBuf);
#else
    memcpy_s(bigImage.bits(), bigImage.sizeInBytes(), imageBuffer.buf, imageBuffer.bytesCountOfBuf);
#endif
    QImage smallImage = bigImage.scaled(thumb.width(), thumb.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    thumb = smallImage;
}

QString WzImageService::createThumbRGB(WzImageBuffer &imageBuffer, QString imageFileName)
{
    QString thumbFileName = WzImageService::getThumbFileName(imageFileName);

    qDebug() << "thumb file: " << thumbFileName;

    if (QFile::exists(thumbFileName))
        return thumbFileName;

    // TODO 改为从配置中读取
    QImage thumb(120, 120, QImage::Format_RGB888);
    createThumbRGB(thumb, imageBuffer);
    thumb.save(thumbFileName, "jpeg");

    return thumbFileName;
}

void WzImageService::imageRotate(WzImageBuffer &imageBuffer, double angle, bool blackBackground)
{
    if (imageBuffer.bitDepth == 8) {
        //imageRotate8bit(imageBuffer, angle, blackBackground);
        return;
    }

    double fx, fy, a, tsin, tcos, cxSrc, cySrc, cxDest, cyDest;
    int dw, dh, width, height;
    uint16_t bColor;
    uint16_t *Vpr1, *dest;
    double *arx1, *arx2;
    double ary1, ary2;

    bColor = blackBackground ? 0 : 65535;
    width = imageBuffer.width;
    height = imageBuffer.height;
    a = angle;
    dw = round(abs((width - 1) * cos(a)) + abs((height - 1) * sin(a)));
    dh = round(abs((width - 1) * sin(a)) + abs((height - 1) * cos(a)));

    dest = new uint16_t[dw * dh];
    memset(dest, 0, dw * dh * 2);

    tsin = sin(a);
    tcos = cos(a);
    cxSrc = (width - 1) / 2;
    cySrc = (height - 1) / 2;
    cxDest = (dw - 1) / 2;
    cyDest = (dh - 1) / 2;
    arx1 = new double[dw];
    arx2 = new double[dw];

    for (int x = 0; x < dw; x++) {
        arx1[x] = cxSrc + (x - cxDest) * tcos;
        arx2[x] = cySrc + (x - cxDest) * tsin;
    }
    Vpr1 = dest;
    for (int y = 0; y < dh; y++) {
        ary1 = (y - cyDest) * tsin;
        ary2 = (y - cyDest) * tcos;
        for (int x = 0; x < dw; x++) {
            fx = arx1[x] - ary1;
            fy = arx2[x] + ary2;
            *Vpr1 = WzUtils::Bilinear(imageBuffer.bit16Array, width, height, fx, fy, bColor);
            Vpr1++;
        }
    }
    delete [] arx1;
    delete [] arx2;

    int imageWidthNew = dw;
    int imageHeightNew = dh;

    int startX = (imageWidthNew - width) / 2;
    int startY = (imageHeightNew - height) / 2;

    /* test code
    if (imageBuffer.buf != nullptr) {
        delete [] imageBuffer.buf;
        imageBuffer.buf = new uint8_t[imageWidthNew * imageHeightNew * 2];
        imageBuffer.width = imageWidthNew;
        imageBuffer.height = imageHeightNew;
        imageBuffer.update();
        memcpy(imageBuffer.buf, dest, imageWidthNew * imageHeightNew * 2);
    }
    */
    uint16_t *buf = reinterpret_cast<uint16_t*>(imageBuffer.buf);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            buf[y * width + x] = dest[(y + startY) * imageWidthNew + startX + x];
        }
    }

    delete []dest;
}

QString WzImageService::createThumb(WzImageBuffer &imageBuffer, QString imageFileName,
                                    const bool isNegative) {
    QString thumbFileName = WzImageService::getThumbFileName(imageFileName);

    qDebug() << "thumb file: " << thumbFileName;

    if (QFile::exists(thumbFileName))
        return thumbFileName;

    WzImageBuffer thumbBuffer;
    thumbBuffer.width = 120;
    thumbBuffer.height = 120; // TODO 改为从配置中读取
#ifdef CHEMI_CAPTURE
    createThumb(thumbBuffer, imageBuffer, 200, isNegative); // TODO 这里的limit改为动态设置
#endif
#ifdef GEL_CAPTURE
    createThumb(thumbBuffer, imageBuffer, 200, false); // TODO 这里的limit改为动态设置
#endif
    thumbBuffer.update();
    QImage thumbJpeg(static_cast<int>(thumbBuffer.width), static_cast<int>(thumbBuffer.height), QImage::Format_Grayscale8);
    memcpy(thumbJpeg.bits(), thumbBuffer.bit8Array, thumbBuffer.bytesCountOfBuf);
    thumbJpeg.save(thumbFileName, "jpeg");

    return thumbFileName;
}

void WzImageService::setMarkerImageName(const QString& markerImageName) {
    if (markerImageName == m_markerImageName) {
        return;
    }
    if (nullptr != m_markerImage) {
        delete m_markerImage;
        m_markerImage = nullptr;
    }
    m_markerImageName = markerImageName;
    m_markerImage = new QImage(markerImageName);
    qDebug("Format of the marker image: %d", m_markerImage->format());
}

bool WzImageService::saveImageAsTiff24(QImage &image, const QString &fileName)
{
    uint8_t *buffer24bit = new uint8_t[image.width() * image.height() * 3];
    uint8_t *buffer24bitPtr = buffer24bit;

    for (uint32_t row = 0; row < image.height(); row++) {
        for (uint32_t col = 0; col < image.width(); col++) {
            auto rgb = image.pixel(col, row);

            *buffer24bitPtr = qRed(rgb);
            buffer24bitPtr++;

            *buffer24bitPtr = qGreen(rgb);
            buffer24bitPtr++;

            *buffer24bitPtr = qBlue(rgb);
            buffer24bitPtr++;
        }
    }
    WzImageBuffer imageBuffer24bit;
    imageBuffer24bit.width = image.width();
    imageBuffer24bit.height = image.height();
    imageBuffer24bit.bitDepth = 8;
    imageBuffer24bit.samplesPerPixel = 3;
    imageBuffer24bit.buf = buffer24bit;
    bool ret = saveImageAsTiff(imageBuffer24bit, fileName);
    delete [] buffer24bit;
    return ret;
}

void WzImageService::updateViewRGB()
{
    if (nullptr != m_image32bit) {
        delete []m_image32bit;
        m_image32bit = nullptr;
    }
    m_image32bit = new uint32_t[static_cast<int>(m_imageBuffer->width * m_imageBuffer->height)];

    int pixelCount = m_imageBuffer->width * m_imageBuffer->height;
    uint8_t *rgbPtr = m_imageBuffer->buf;
    for (int i = 0; i < pixelCount; i++) {
        uint8_t red = *rgbPtr; rgbPtr++;
        uint8_t green = *rgbPtr; rgbPtr++;
        uint8_t blue = *rgbPtr; rgbPtr++;
        m_image32bit[i] = (0xff << 24) | (red << 16) | (green << 8) | blue;
    }

    g_imageView->updateImage(m_image32bit, m_imageBuffer->width, m_imageBuffer->height,
                             m_imageBuffer);
}

bool WzImageService::isRgbImage() const
{
    if (m_imageBuffer) {
        return m_imageBuffer->bitDepth == 8 && m_imageBuffer->samplesPerPixel == 3;
    }
    return false;
}

void WzImageService::scanPalettePath(const QString& path) {
    qInfo() << "scanPalettePath: " << path;
    m_pseudoList.clear();
    makeGrayPalette();
    loadDefaultPalette();

    QDir directory(path);
    QStringList filenames = directory.entryList(QStringList() << "*.shpal", QDir::Files);
    foreach(QString filename, filenames) {
        QFileInfo fi(QDir(path), filename);
        QFile file(fi.absoluteFilePath());
        if(!file.open(QIODevice::ReadOnly)) {
            qWarning() << "scanPalettePath error:" << file.errorString();
            continue;
        }

        QTextStream in(&file);

        QVariantMap pseudo;
        QString pseudoName = "";
        QVariantList rgbList;

        if(!in.atEnd()) {
            pseudoName = in.readLine();
        }
        while(!in.atEnd()) {
            QString line = in.readLine();
            QStringList fields = line.split(" ");
            QMap<QString, QVariant> rgb;
            if (fields.count() == 3) {
                rgb["R"] = fields[0].toInt();
                rgb["G"] = fields[1].toInt();
                rgb["B"] = fields[2].toInt();
                rgbList.append(rgb);
            }
        }
        if (pseudoName != "" && rgbList.count() > 0) {
            pseudo["name"] = pseudoName;
            pseudo["rgbList"] = rgbList;

            //qDebug() << qPrintable("const char Pseudo" + pseudoName + "[] = ") << QJsonDocument::fromVariant(pseudo).toJson(QJsonDocument::Compact) + ";";

            m_pseudoList.append(pseudo);
        }
    }

    emit pseudoListChanged(getPseudoList());
}

void WzImageService::makeGrayPalette() {
    QVariantMap pseudo;
    QVariantList rgbList;
    for (int n = 0; n < 256; n++) {
        QVariantMap rgb;
        rgb["R"] = 255-n;
        rgb["G"] = 255-n;
        rgb["B"] = 255-n;
#ifdef CHEMI_CAPTURE
        rgbList.append(rgb);
#endif
#ifdef GEL_CAPTURE
        rgbList.append(rgb);
#endif
    }
    pseudo["name"] = "Gray";
    pseudo["rgbList"] = rgbList;
    m_pseudoList.append(pseudo);
}

void WzImageService::loadDefaultPalette() {

    m_pseudoList.append(QJsonDocument::fromJson(kPseudoCoomassie).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoEtBr).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoFalseColor).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoFlamingo).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoGoldSilver).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoPseudo).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoSilver).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoSpectrum).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoStainFree).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoSYBRGree).toVariant());
    m_pseudoList.append(QJsonDocument::fromJson(kPseudoSYPRORuby).toVariant());

}

QJsonArray WzImageService::getPseudoList() {
    return QJsonArray::fromVariantList(m_pseudoList);
}

void WzImageService::drawColorBar(QImage &image)
{
    if (!g_imageView)
        return;

    QVariant colorTable = g_imageView->getColorTable();
    QList<QVariant> rgbList = colorTable.toList();

    QImage colorBar(1, 256, QImage::Format_ARGB32);
    uint32_t* bits = reinterpret_cast<uint32_t*>(colorBar.bits());
    int colorCount = rgbList.count();
    if (colorCount > 256)
        colorCount = 256;
    for (int i = 0; i < colorCount; i++) {
        QMap<QString, QVariant> map = rgbList[i].toMap();
        *bits = 0xff << 24 | map["R"].toInt() << 16 | map["G"].toInt() << 8 | map["B"].toInt();
        bits++;
    }

    int colorBarHeight = image.height() * 0.5;
    int colorBarWidth = colorBarHeight * 0.078;
    QRect colorBarRect(image.width() - colorBarWidth,
                       image.height() - colorBarHeight,
                       colorBarWidth,
                       colorBarHeight);

    QPainter painter;
    painter.begin(&image);
    painter.drawImage(colorBarRect, colorBar);

    painter.setCompositionMode(QPainter::CompositionMode_Exclusion);
    QPen pen(qRgb(255, 255, 255));
    QFont font("Arial", colorBarWidth / 2);
    QFontMetrics fm(font);
    painter.setFont(font);
    painter.setPen(pen);

    QString grayHighStr = QString::number(m_high);
    QString grayLowStr = QString::number(m_low);
    QString grayMiddleStr = QString::number((m_high - m_low) / 2 + m_low);

    painter.drawText(image.width() - colorBarWidth - 5 - fm.width(grayHighStr),
                     image.height() - colorBarHeight + fm.height() - 4,
                     grayHighStr);
    painter.drawText(image.width() - colorBarWidth - 5 - fm.width(grayLowStr),
                     image.height() - 4,
                     grayLowStr);
    painter.drawText(image.width() - colorBarWidth - 5 - fm.width(grayMiddleStr),
                     image.height() - colorBarHeight / 2,
                     grayMiddleStr);
    painter.end();
}

void WzImageService::printPreview(QPrinter *printer) {
    QPainter painter;
    painter.begin(printer);

    // 生成可打印矩形
    qreal marginLeft, marginRight, marginTop, marginBottom;
    printer->getPageMargins(&marginLeft, &marginTop, &marginRight, &marginBottom, QPrinter::DevicePixel);
    QRect printRect;
    printRect.setLeft(marginLeft);
    printRect.setTop(marginTop);
    printRect.setWidth(printer->pageRect().width() - marginLeft - marginRight);
    printRect.setHeight(printer->pageRect().height() - marginBottom - marginTop);

    QImage* printImage = nullptr;
    // 如果可打印区域的方向(横/竖)与图片不一致则旋转图片
    if (printRect.width() > printRect.height()) {
        if (m_printImage->width() < m_printImage->height()) {
            QPoint center = m_printImage->rect().center();
            QMatrix matrix;
            matrix.translate(center.x(), center.y());
            matrix.rotate(90);
            printImage = new QImage(m_printImage->transformed(matrix));
        } else {
            printImage = new QImage(*m_printImage);
        }
    } else {
        if (m_printImage->width() > m_printImage->height()) {
            QPoint center = m_printImage->rect().center();
            QMatrix matrix;
            matrix.translate(center.x(), center.y());
            matrix.rotate(90);
            printImage = new QImage(m_printImage->transformed(matrix));
        } else {
            printImage = new QImage(*m_printImage);
        }
    }

    // 判断图片尺寸是否超出了可打印区域
    if (printImage->width() > printRect.width() || printImage->height() > printRect.height()) {
        qreal imageAspectRatio = static_cast<qreal>(printImage->width()) / static_cast<qreal>(printImage->height());
        qreal imageNewHeight = printImage->height();
        qreal imageNewWidth = printImage->width();

        if (printImage->width() > printRect.width()) {
            imageNewWidth = printRect.width();
            imageNewHeight = imageNewWidth / imageAspectRatio;
        }

        if (imageNewHeight > printRect.height()) {
            imageNewHeight = printRect.height();
            imageNewWidth = imageNewHeight * imageAspectRatio;
        }

        QImage newImage = printImage->scaled(imageNewWidth, imageNewHeight);
        delete printImage;
        printImage = new QImage(newImage);
    }

    QPoint printPoint;
    printPoint.setX((printRect.width() - printImage->width()) / 2);
    printPoint.setY((printRect.height() - printImage->height()) / 2);
    printPoint.setX(printPoint.x() + marginLeft);
    printPoint.setY(printPoint.y() + marginTop);

    painter.drawImage(printPoint, *printImage);
    delete printImage;

    painter.end();
}
