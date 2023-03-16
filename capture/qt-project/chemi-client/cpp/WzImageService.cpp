#include "WzImageService.h"

WzImageService* g_imageService = nullptr;

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
    qDebug() << "WzImageService created";
    if (!g_imageService)
        g_imageService = this;

    m_markerBuffer = new WzImageBuffer;

    scanPalettePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + kPaletteFolder);

    m_colorChannels = new WzColorChannel*[COLOR_CHANNEL_COUNT];
    for (int i = 0; i < COLOR_CHANNEL_COUNT; i++)
        m_colorChannels[i] = new WzColorChannel();


    connect(g_localServer, &WzLocalServer::action, this, &WzImageService::localServerAction);
}

WzImageService::~WzImageService() {
    if (nullptr != m_colorChannels) {
        for (int i = 0; i < COLOR_CHANNEL_COUNT; i++) {
            if (nullptr != m_colorChannels[i]) {
                delete m_colorChannels[i];
                m_colorChannels[i] = nullptr;
            }
        }

        delete [] m_colorChannels;
        m_colorChannels = nullptr;
    }

    if (nullptr != m_markerBuffer) {
        if (nullptr != m_markerBuffer->buf) {
            delete [] m_markerBuffer->buf;
            m_markerBuffer->buf = nullptr;
        }
        delete m_markerBuffer;
        m_markerBuffer = nullptr;
    }
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
    WzImageReader imageReader;
    WzEnum::ErrorCode ec = imageReader.loadImage(imageFileName, imageBuffer);
	if (WzEnum::Success != ec)
		return ec;
    
    imageBuffer->update();
    imageBuffer->imageThumbFile = createThumb(*imageBuffer, imageFileName);

    return WzEnum::Success;
}

bool WzImageService::saveImageAsTiff(const WzImageBuffer& imageBuffer, const QString& filename) {
    QByteArray filenameByteArray = filename.toLocal8Bit();
    char* fn = filenameByteArray.data();

    TIFF* tif = TIFFOpen(fn, "w");
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

        qInfo("TIFF Saved: %s", fn);
    }

    std::string fn2(fn);
    std::shared_ptr<Exiv2::Image> image = Exiv2::ImageFactory::open(fn2);
    if (image.get()) {
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        exifData["Exif.Image.ExposureTime"] = Exiv2::Rational(imageBuffer.exposureMs, 1000);
        exifData["Exif.Photo.ExposureTime"] = Exiv2::Rational(imageBuffer.exposureMs, 1000);
        QString dateTimeStr = imageBuffer.captureDateTime.toString("yyyy:MM:dd hh:mm:ss");
        exifData["Exif.Image.DateTimeOriginal"] = dateTimeStr.toStdString();
        exifData["Exif.Photo.DateTimeOriginal"] = dateTimeStr.toStdString();
        image->setExifData(exifData);
        image->writeMetadata();
    } else {
        qWarning() << "exiv2.open error";
    }

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
    m_low = low;
    m_high = high;
}

void WzImageService::updateLowHighMarker(const int &low, const int &high)
{
    int l = low;
    int h = high;

    if (l < 0) l = 0;
    if (h > 65535) h = 65535;

    if (l > h) l = h;
    if (l == h) {
        qWarning() << "updateLowHighMarker, low与high相同";
        for(int i=0; i<65536; i++)
            m_grayTableMarker[i] = 127; // 灰色显示
        return;
    }
    for(int i=0; i<=l; i++) {
        m_grayTableMarker[i] = 0;
    }
    for(int i=h; i<=65535; i++) {
        m_grayTableMarker[i] = 255;
    }
    int j = h - l;
    if (j == 1) {
        qWarning() << "updateLowHighMarker, low与high只相差了1";
        return;
    }
    double scale = static_cast<double>(j+1) / 256.0;
    for(int i=l; i<=h; i++) {
        double k = ((i-l) / scale);
        if (k < 0)
            k = 0;
        else if (k > 255)
            k = 255;
        m_grayTableMarker[i] = static_cast<unsigned char>(k);
    }
    m_markerBuffer->low = low;
    m_markerBuffer->high = high;
    m_lowMarker = low;
    m_highMarker = high;
};

// TODO 改为异步
void WzImageService::calculateLowHigh() {
    int low, high;
    if (nullptr == m_imageBuffer || m_imageBuffer->bytesCountOfBuf == 0) {
        qWarning() << "calculateLowHigh, 图片缓冲区无内容";
        lowHighCalculated(WzEnum::NullBuffer, 0, 0);
        return;
    }
    m_imageBuffer->update();
    m_imageBuffer->getMinMax(low, high);
    m_low = low;
    m_high = high;
    lowHighCalculated(WzEnum::Success, low, high);
}

void WzImageService::calculateLowHighMarker()
{
    int low, high;
    if (nullptr == m_markerBuffer || m_markerBuffer->bytesCountOfBuf == 0) {
        qWarning() << "calculateLowHighMarker, Marker 缓冲区无内容";
        lowHighMarkerCalculated(WzEnum::NullBuffer, 0, 0);
        return;
    }
    m_markerBuffer->update();
    m_markerBuffer->getMinMax(low, high);
    m_lowMarker = low;
    m_highMarker = high;
    lowHighMarkerCalculated(WzEnum::Success, low, high);
}

void WzImageService::calculateLowHighRGB(const int channel)
{
    int low, high;
    WzImageBuffer *imgBuf = m_colorChannels[channel]->imageBuffer();
    if (imgBuf && imgBuf->buf) {
        imgBuf->update();
        imgBuf->getMinMax(low, high);
        emit lowHighRGBCalculated(WzEnum::Success, low, high);
    } else {
        qWarning() << "calculateLowHighRGB, imageBuffer == nullptr";
        emit lowHighRGBCalculated(WzEnum::NullBuffer, 0, 0);
    }
}

QVariantMap WzImageService::getActiveImageInfo() {
    QVariantMap imageInfo;

    if (nullptr == m_imageBuffer)
        return imageInfo;

    // TODO 字段待补全
    imageInfo["imageFile"] = m_imageBuffer->imageFile;
    imageInfo["imageThumbFile"] = m_imageBuffer->imageThumbFile;
    imageInfo["openedLight"] = "";
    imageInfo["colorChannel"] = 0;

    return imageInfo;
};

void WzImageService::deleteImage(const QString imageInfoJsonStr) {
    QJsonDocument imageInfo = QJsonDocument::fromJson(imageInfoJsonStr.toUtf8());
    QFile imageFile(imageInfo["imageFile"].toString());
    if (imageFile.exists())
        imageFile.remove();

    QFile imageThumbFile(imageInfo["imageThumbFile"].toString());
    if (imageThumbFile.exists())
        imageThumbFile.remove();

    QFile imageWhiteFile(imageInfo["imageWhiteFile"].toString());
    if (imageWhiteFile.exists())
        imageWhiteFile.remove();
}

void WzImageService::closeActiveImage() {
    uint8_t* image8bit = new uint8_t[800*600];
    memset(image8bit, 0x7f, 800 * 600);
    if (m_imageView)
        m_imageView->updateImage(image8bit, 800, 600);
    delete [] image8bit;
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

bool WzImageService::saveAsImage(const QString &fileName, const QString &format) {
    if (format == "tiff16") {
        return saveImageAsTiff(*m_imageBuffer, fileName);
    }
    if (nullptr == m_imageView) {
        qWarning("m_imageView == nullptr");
        return false;
    }
    QImage* img = m_imageView->paintImage();
    if (nullptr == img) {
        qWarning("m_imageView->image() == nullptr");
        return false;
    }
    if (m_isPseudoColorBarInImage)
        drawColorBar(*img);
    if (format == "tiff24") {
        return saveImageAsTiff24(*img, fileName);
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
        imageBuffer8bit.bitDepth = 8;
        imageBuffer8bit.buf = m_image8bit;
        return saveImageAsTiff(imageBuffer8bit, fileName);
    }

    return false;
}

bool WzImageService::printImage(const QString& language) {
    qInfo("printImage");
    if (m_imageView == nullptr) {
        qInfo("imageView == nullptr");
        return false;
    }
    QImage* img = m_imageView->paintImage();
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

    QList<QToolBar*> toolbarList = previewDialog.findChildren<QToolBar*>();
    if(!toolbarList.isEmpty()) {
        QList<QAction*> actions = toolbarList.first()->actions();
        if (language == "zh") {
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

QJsonObject WzImageService::analysisImage(const QJsonObject &imageInfo)
{
    QJsonObject result;
    qInfo() << "analysisImage:" << imageInfo;
#ifdef Q_OS_WIN
    QDir dir(qApp->applicationDirPath());
    QFileInfo fi(dir, kAnalysisName);
    if (!fi.exists()) {
        result["code"] = -1;
        result["msg"] = tr("分析软件没有找到");
    } else {
        QString imageFile = imageInfo["imageFile"].toString();
        QStringList args;
        args << imageFile;

        if (imageInfo["imageInvert"].toInt() == 2 ||
            imageInfo["imageInvert"].toInt() == 4) {
        } else {
            args << "--imageInvert";
        }

        args << QString("--grayLow=%1").arg(round(imageInfo["grayLow"].toDouble()));
        args << QString("--grayHigh=%1").arg(round(imageInfo["grayHigh"].toDouble()));
        args << QString("--serverPort=%1").arg(g_localServer->serverName());

        QProcess::startDetached(fi.absoluteFilePath(), args);
        result["code"] = 0;
    }
#else
    result["code"] = -1;
    result["msg"] = tr("当前操作系统尚不支持该操作");
#endif
    return result;
}

void WzImageService::assignGlobalImageView()
{
    setImageView(g_imageView);
}

void WzImageService::setColorChannelFile(const QString &fileName, const int colorChannel)
{
    m_colorChannels[colorChannel]->setImageFile(fileName);
}

void WzImageService::setColorChannelLowHigh(const int colorChannel, const int low, const int high)
{
    m_colorChannels[colorChannel]->setLow(low);
    m_colorChannels[colorChannel]->setHigh(high);
}

QString WzImageService::getColorChannelFile(const int colorChannel)
{
    return m_colorChannels[colorChannel]->imageFile();
}

QVariantMap WzImageService::getPaletteByName(const QString &paletteName)
{
    for (int i = 0; i < m_pseudoList.count(); i++) {
        if (m_pseudoList[i].toMap()["name"] == paletteName)
            return m_pseudoList[i].toMap();
    }
    return QVariantMap();
}

void WzImageService::updateView() {
    if(!m_imageView)
        return;

    if (m_isColorChannel) {
        bool isHasImageFile = false;
        for (int i = 1; i < COLOR_CHANNEL_COUNT; i++) {
            if (m_colorChannels[i]->imageFile() != "") {
                isHasImageFile = true;
                break;
            }
        }
        if (isHasImageFile) {
            updateViewRGB();
            return;
        }
    }

    if(!m_imageBuffer->buf)
        return;

    QImage markerImage;
    uchar* markerData = nullptr;
    bool showMarker = m_showMarker;
    if (showMarker && nullptr != m_markerImage) {
        markerImage = m_markerImage->scaled(static_cast<int>(m_imageBuffer->width),
                                            static_cast<int>(m_imageBuffer->height),
                                            Qt::IgnoreAspectRatio);
        markerData = markerImage.bits();
    } else if (showMarker && nullptr != m_markerBuffer->buf) {
    } else {
        showMarker = false;
    }

    WzUtils::new8bitBuf(&m_imageChemi8bit, m_imageBuffer->width, m_imageBuffer->height);
    WzUtils::new8bitBuf(&m_imageMarker8bit, m_imageBuffer->width, m_imageBuffer->height);
    WzUtils::new8bitBuf(&m_image8bit, m_imageBuffer->width, m_imageBuffer->height);
    WzUtils::new32bitBuf(&m_image32bit, m_imageBuffer->width, m_imageBuffer->height);

    int pixelCount = m_imageBuffer->width * m_imageBuffer->height;
    m_imageBuffer->update();
    uint16* pWord = m_imageBuffer->bit16Array;

    updateColorTable();

    if (!showMarker) {
        if (m_invert)
            for(int i=0; i<pixelCount; i++) {
                uint8_t gray = 255 - static_cast<char>(m_grayTable[pWord[i]]);
                uint8_t r = m_colorTableR[gray];
                uint8_t g = m_colorTableG[gray];
                uint8_t b = m_colorTableB[gray];
                m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                m_image8bit[i] = gray;
            }
        else
            for(int i=0; i<pixelCount; i++) {
                uint8_t gray = static_cast<char>(m_grayTable[pWord[i]]);
                uint8_t r = m_colorTableR[gray];
                uint8_t g = m_colorTableG[gray];
                uint8_t b = m_colorTableB[gray];
                m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                m_image8bit[i] = gray;
            }
    } else {
        if (m_showChemi) {
            if (nullptr != m_markerBuffer->buf) {
                uint16_t* markerBufPtr = reinterpret_cast<uint16_t*>(m_markerBuffer->buf);
                // 使用了伪彩调色板
                if (m_isPseudoColor) {
                    for(int i=0; i<pixelCount; i++) {
                        int gray = m_grayTable[pWord[i]];
                        int alphaLevel = gray + 1;
                        int markerGray = m_grayTableMarker[markerBufPtr[i]] * (256 - alphaLevel);
                        uint8_t r = (m_colorTableR[255 - gray] * alphaLevel + markerGray) >> 8;
                        uint8_t g = (m_colorTableG[255 - gray] * alphaLevel + markerGray) >> 8;
                        uint8_t b = (m_colorTableB[255 - gray] * alphaLevel + markerGray) >> 8;
                        m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                        int overlapGray8bit = m_grayTableMarker[markerBufPtr[i]] - gray;
                        if (overlapGray8bit < 0) overlapGray8bit = 0;
                        m_image8bit[i] = overlapGray8bit;
                        m_imageChemi8bit[i] = 255 - gray;
                        m_imageMarker8bit[i] = m_grayTableMarker[markerBufPtr[i]];
                    }
                } else {
                    for(int i=0; i<pixelCount; i++) {
                        int gray;
                        if (pWord[i] < m_low)
                            gray = m_grayTableMarker[markerBufPtr[i]];
                        else
                            gray = m_grayTableMarker[markerBufPtr[i]] - m_grayTable[pWord[i]];
                        if (gray < 0) gray = 0;
                        m_image32bit[i] = (0xff << 24) | (gray << 16) | (gray << 8) | gray;
                        m_image8bit[i] = gray;
                        m_imageChemi8bit[i] = 255 - m_grayTable[pWord[i]];
                        m_imageMarker8bit[i] = m_grayTableMarker[markerBufPtr[i]];
                    }
                }
            } else {
                for(int i=0; i<pixelCount; i++) {
                    int gray = (markerData ? markerData[i] : 0) - m_grayTable[*pWord];
                    if (gray < 0) gray = 0;
                    uint8_t r = m_colorTableR[gray];
                    uint8_t g = m_colorTableG[gray];
                    uint8_t b = m_colorTableB[gray];
                    m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                    m_image8bit[i] = gray;
                }
            }
        } else {
            if (nullptr != m_markerBuffer->buf) {
                uint16_t* bit16Ptr = reinterpret_cast<uint16_t*>(m_markerBuffer->buf);
                for(int i=0; i<pixelCount; i++) {
                    uint8_t gray = static_cast<char>(m_grayTableMarker[bit16Ptr[i]]);
                    m_image32bit[i] = (0xff << 24) | (gray << 16) | (gray << 8) | gray;
                    m_image8bit[i] = gray;
                }
            } else {
                for(int i=0; i<pixelCount; i++) {
                    uint8_t gray = markerData ? markerData[i] : 0;
                    m_image32bit[i] = (0xff << 24) | (gray << 16) | (gray << 8) | gray;
                    m_image8bit[i] = gray;
                }
            }
        }
    }

    if (m_imageView)
        m_imageView->updateImage(m_image32bit, m_imageBuffer->width, m_imageBuffer->height);
};

void WzImageService::updateCurrentImage(const WzImageBuffer& imageBuffer) {
    // 原有缓冲区有数据且数据大小与新的不同时则先修改内存大小
    if (m_imageBuffer->buf && m_imageBuffer->bytesCountOfBuf != imageBuffer.bytesCountOfBuf) {
        delete []m_imageBuffer->buf;
        m_imageBuffer->buf = new uint8_t[imageBuffer.bytesCountOfBuf];
        m_imageBuffer->bytesCountOfBuf = imageBuffer.bytesCountOfBuf;
    }
    if (m_imageBuffer) {
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

    thumbBuffer.bytesCountOfBuf = realThumbHeight * realThumbWidth;
    thumbBuffer.width = realThumbWidth;
    thumbBuffer.height = realThumbHeight;

    std::vector<uint> thumbBuffer16bit = std::vector<uint>(realThumbWidth * realThumbHeight);
    std::vector<uint>::iterator thumbBuffer16bitPtr = thumbBuffer16bit.begin();    

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
            thumbBuffer16bit.at(thumbRow * realThumbWidth + thumbCol) = grayAccumulate / (scaleInt * scaleInt);
            //thumbBuffer16bitPtr++;
        }
    }

    // 找最大，最小灰度值
    thumbBuffer16bitPtr = thumbBuffer16bit.begin();
    uint maxGray = *thumbBuffer16bitPtr;
    uint minGray = *thumbBuffer16bitPtr;
    for (uint thumbPixel = 0; thumbPixel < realThumbWidth * realThumbHeight; thumbPixel++) {
        maxGray = std::max(maxGray, *thumbBuffer16bitPtr);
        minGray = std::min(minGray, *thumbBuffer16bitPtr);
        thumbBuffer16bitPtr++;
    }
    maxGray = std::max<uint>(thumbMaxGrayLimit, maxGray);

    // 16位灰阶转8位
    if (maxGray > minGray) {
        thumbBuffer16bitPtr = thumbBuffer16bit.begin();
        double grayScale = static_cast<double>(255) / static_cast<double>((maxGray - minGray));
        thumbBuffer.bitDepth = 8;
        thumbBuffer.buf = new uint8_t[thumbBuffer.bytesCountOfBuf];
        thumbBuffer.update();
        uint8_t* thumbBufferPtr = thumbBuffer.bit8Array;
        for (uint i = 0; i < realThumbHeight * realThumbWidth; i++) {
            if (isNegative)
                *thumbBufferPtr = 255 - static_cast<unsigned char>((*thumbBuffer16bitPtr - minGray) * grayScale);
            else
                *thumbBufferPtr = static_cast<unsigned char>((*thumbBuffer16bitPtr - minGray) * grayScale);
            thumbBufferPtr++;
            thumbBuffer16bitPtr++;
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
            thumbBuffer16bitPtr++;
        }
    }
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

    createThumb(thumbBuffer, imageBuffer, 200, isNegative); // TODO 这里的limit改为动态设置

    thumbBuffer.update();
    QImage thumbJpeg(static_cast<int>(thumbBuffer.width), static_cast<int>(thumbBuffer.height), QImage::Format_Grayscale8);
    memcpy(thumbJpeg.bits(), thumbBuffer.bit8Array, thumbBuffer.bytesCountOfBuf);
    thumbJpeg.save(thumbFileName, "jpeg");

    return thumbFileName;
}

int WzImageService::getLow() const
{
    return m_low;
}

int WzImageService::getHigh() const
{
    return m_high;
}

int WzImageService::getLowMarker() const
{
    return m_lowMarker;
}

int WzImageService::getHighMarker() const
{
    return m_highMarker;
}

void WzImageService::setLow(int low)
{
    m_low = low;
}

void WzImageService::setHigh(int high)
{
    m_high = high;
}

void WzImageService::setLowMarker(int lowMarker)
{
    m_lowMarker = lowMarker;
}

void WzImageService::setHighMarker(int highMarker)
{
    m_highMarker = highMarker;
}

void WzImageService::updateViewRGB()
{
    uint32_t imageWidth = 0;
    uint32_t imageHeight = 0;

    if (m_colorChannels[WzEnum::Red]->imageFile() != "") {
        imageWidth = m_colorChannels[WzEnum::Red]->imageBuffer()->width;
        imageHeight = m_colorChannels[WzEnum::Red]->imageBuffer()->height;
    }

    if (m_colorChannels[WzEnum::Green]->imageFile() != "") {
        imageWidth  = qMax(imageWidth, m_colorChannels[WzEnum::Green]->imageBuffer()->width);
        imageHeight = qMax(imageHeight, m_colorChannels[WzEnum::Green]->imageBuffer()->height);
    }

    if (m_colorChannels[WzEnum::Blue]->imageFile() != "") {
        imageWidth  = qMax(imageWidth, m_colorChannels[WzEnum::Blue]->imageBuffer()->width);
        imageHeight = qMax(imageHeight, m_colorChannels[WzEnum::Blue]->imageBuffer()->height);
    }

    WzImageBuffer* r = m_colorChannels[WzEnum::Red]->imageBuffer();
    WzImageBuffer* g = m_colorChannels[WzEnum::Green]->imageBuffer();
    WzImageBuffer* b = m_colorChannels[WzEnum::Blue]->imageBuffer();

    if (r->buf) {
        r->updateGray16Table(m_colorChannels[WzEnum::Red]->low(), m_colorChannels[WzEnum::Red]->high());
    }
    if (g->buf) {
        g->updateGray16Table(m_colorChannels[WzEnum::Green]->low(), m_colorChannels[WzEnum::Green]->high());
    }
    if (b->buf) {
        b->updateGray16Table(m_colorChannels[WzEnum::Blue]->low(), m_colorChannels[WzEnum::Blue]->high());
    }

    uint32_t *imageData = new uint32_t[imageWidth * imageHeight];
    memset(imageData, 0, imageWidth * imageHeight * sizeof(uint32_t));

    for (uint32_t row = 0; row < imageHeight; row++) {
        for (uint32_t col = 0; col < imageWidth; col++) {
            uint32_t idx = row * imageWidth + col;
            imageData[idx] = 0xFF000000;
            if (r->buf) {
                if (row < r->height && col < r->width) {
                    uint8_t gray = 0;
                    r->getPixelAs8bit(row, col, gray);
                    imageData[idx] = imageData[idx] | (gray << 16);
                }
            }
            if (g->buf) {
                if (row < g->height && col < g->width) {
                    uint8_t gray = 0;
                    g->getPixelAs8bit(row, col, gray);
                    imageData[idx] = imageData[idx] | (gray << 8);
                }
            }
            if (b->buf) {
                if (row < b->height && col < b->width) {
                    uint8_t gray = 0;
                    b->getPixelAs8bit(row, col, gray);
                    imageData[idx] = imageData[idx] | gray;
                }
            }
        }
    }

    m_imageView->updateImage(imageData, imageWidth, imageHeight);
    delete [] imageData;
}

bool WzImageService::getIsColorChannel() const
{
    return m_isColorChannel;
}

void WzImageService::setIsColorChannel(bool isColorChannel)
{
    m_isColorChannel = isColorChannel;
}

void WzImageService::updateColorTable()
{
    if (m_imageView) {
        QList<QVariant> list = m_imageView->getColorTable().toList();
        updateColorTable(list);
    } else {
        for (int i = 0; i < 256; i++) {
            m_colorTableR[i] = i;
            m_colorTableG[i] = i;
            m_colorTableB[i] = i;
        }
    }
}

void WzImageService::updateColorTable(const QList<QVariant> &rgbList)
{
    int m = rgbList.count();
    if (m > 256) m = 256;
    for (int n = 0; n < m; n++) {
        QMap<QString, QVariant> map = rgbList[n].toMap();
        m_colorTableR[n] = map["R"].toInt();
        m_colorTableG[n] = map["G"].toInt();
        m_colorTableB[n] = map["B"].toInt();
    }
}

bool WzImageService::getIsPseudoColorBarInImage() const
{
    return m_isPseudoColorBarInImage;
}

void WzImageService::setIsPseudoColorBarInImage(bool isPseudoColorBarInImage)
{
    m_isPseudoColorBarInImage = isPseudoColorBarInImage;
}

bool WzImageService::updateColorTable(const QString &paletteName)
{
    foreach(QVariant item, m_pseudoList) {
        auto map = item.toMap();
        if (map["name"].toString() == paletteName) {
            if (m_imageView)
                m_imageView->setColorTable(map["rgbList"].toList());
            return true;
        }
    }
    return false;
}

bool WzImageService::getIsPseudoColor() const
{
    return m_isPseudoColor;
}

void WzImageService::setIsPseudoColor(bool isPseudoColor)
{
    m_isPseudoColor = isPseudoColor;
}

void WzImageService::drawColorBar(QImage &image)
{
    QImage colorBar(1, 256, QImage::Format_ARGB32);
    uint32_t* bits = reinterpret_cast<uint32_t*>(colorBar.bits());
    for (int i = 0; i < 256; i++) {
        *bits = 0xff << 24 | m_colorTableR[i] << 16 | m_colorTableG[i] << 8 | m_colorTableB[i];
        bits++;
    }

    int colorBarHeight = image.height() * 0.5;
    int colorBarWidth = colorBarHeight * 0.078;
    QRect colorBarRect(image.width() - colorBarWidth,
                       image.height() - colorBarHeight,
                       colorBarWidth,
                       colorBarHeight);

    QPainter painter(&image);

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
}

bool WzImageService::saveImageAsTiff24(QImage &image, const QString &fileName)
{
    return saveImageAsTiff24(image, m_imageBuffer->width, m_imageBuffer->height, fileName);
}

bool WzImageService::saveImageAsTiff24(QImage &image, const int imageWidth, const int imageHeight, const QString &filename)
{
    uint8_t *buffer24bit = new uint8_t[imageWidth * imageHeight * 3];
    uint8_t *buffer24bitPtr = buffer24bit;
    for (int row = 0; row < imageHeight; row++) {
        for (int col = 0; col < imageWidth; col++) {
            int rgb = image.pixel(col, row);
            *buffer24bitPtr = qRed(rgb);
            buffer24bitPtr++;
            *buffer24bitPtr = qGreen(rgb);
            buffer24bitPtr++;
            *buffer24bitPtr = qBlue(rgb);
            buffer24bitPtr++;
        }
    }
    WzImageBuffer imageBuffer24bit;
    imageBuffer24bit.width = imageWidth;
    imageBuffer24bit.height = imageHeight;
    imageBuffer24bit.bitDepth = 8;
    imageBuffer24bit.samplesPerPixel = 3;
    imageBuffer24bit.buf = buffer24bit;
    bool ret = saveImageAsTiff(imageBuffer24bit, filename);
    delete [] buffer24bit;
    return ret;
}

WzImageView *WzImageService::getImageView() const
{
    return m_imageView;
}

void WzImageService::setImageView(WzImageView *imageView)
{
    m_imageView = imageView;
}

bool WzImageService::getInvert() const
{
    return m_invert;
}

void WzImageService::setInvert(bool invert)
{
    m_invert = invert;
}

bool WzImageService::getShowMarker() const
{
    return m_showMarker;
}

void WzImageService::setShowMarker(bool showMarker)
{
    m_showMarker = showMarker;
}

bool WzImageService::getShowChemi() const
{
    return m_showChemi;
}

void WzImageService::setShowChemi(bool showChemi)
{
    m_showChemi = showChemi;
}

void WzImageService::setMarkerImageName(const QString& markerImageName) {
    if (markerImageName == m_markerImageName) {
        return;
    }
    if (nullptr != m_markerImage) {
        delete m_markerImage;
        m_markerImage = nullptr;
    }
    if (nullptr != m_markerBuffer->buf) {
        delete [] m_markerBuffer->buf;
        m_markerBuffer->buf = nullptr;
    }
    m_markerImageName = markerImageName;
    qInfo() << "marker file:" << m_markerImageName;
    if (markerImageName.endsWith(".tif")) {
        WzEnum::ErrorCode ec = loadImage(m_markerImageName, m_markerBuffer);
        if (ec != WzEnum::Success) {
            qInfo("load marker error: %d", ec);
        } else {
            qInfo("load tiff marker success");
        }
    } else {
        m_markerImage = new QImage(markerImageName);
        qDebug("Format of the marker image: %d", m_markerImage->format());
    }
}

uint8_t *WzImageService::getImage8bit() const
{
    return m_image8bit;
}

WzImageBuffer *WzImageService::getImageBuffer() const
{
    return m_imageBuffer;
}

uint8_t *WzImageService::getImageChemi8bit() const
{
    return m_imageChemi8bit;
}

uint8_t *WzImageService::getImageMarker8bit() const
{
    return m_imageMarker8bit;
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

void WzImageService::localServerAction(const QJsonDocument &jsonDocument)
{
    qInfo() << "localServerAction, " << jsonDocument.toJson();
    QString action = jsonDocument["action"].toString();
    if (action == "imageOpened") {
        emit analysisImageOpened();
    }
}

