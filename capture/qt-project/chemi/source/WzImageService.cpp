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
    g_imageService = this;
    qDebug() << "WzImageService created";

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
    WzUtils::delete32bitBuf(&m_image32bit);
    WzUtils::delete8bitBuf(&m_image8bit);
    WzUtils::delete8bitBuf(&m_imageChemi8bit);
    WzUtils::delete8bitBuf(&m_imageMarker8bit);
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
    if (m_imageBuffer)
        m_imageBuffer->imageFile = localFile;
    emit imageOpened(ret);
}

WzEnum::ErrorCode WzImageService::loadImage(const QString& imageFileName, WzImageBuffer* imageBuffer) {
    WzImageReader imageReader;
    WzEnum::ErrorCode ec = imageReader.loadImage(imageFileName, imageBuffer);
	if (WzEnum::Success != ec)
		return ec;
    
    if (imageBuffer) {
        imageBuffer->update();
        imageBuffer->imageThumbFile = createThumb(*imageBuffer, imageFileName);
    } else {
        qWarning() << "ImageService::loadImage, imageBuffer == nullptr";
    }

    return WzEnum::Success;
}

bool WzImageService::ImageFlipBlackWhite(QImage& image,QString& path){
    QByteArray filenameByteArray = path.toLocal8Bit();
    char* fn = filenameByteArray.data();
    QString path_white=path.left(path.size()-4)+"_white.tif";
    QByteArray filenameByteArray_white = path_white.toLocal8Bit();
    char* fn_white = filenameByteArray_white.data();

    TIFF* tif_in = TIFFOpen(fn, "r");
        if (!tif_in) {
            return 1;
        }

        // Read the input TIFF image dimensions
        uint32_t width, height;
        float xres = 96.,yres = 96.;
        TIFFGetField(tif_in, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tif_in, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField(tif_in, TIFFTAG_XRESOLUTION, &xres);
        TIFFGetField(tif_in, TIFFTAG_YRESOLUTION, &yres);

        // Open the output TIFF file
        TIFF* tif_out = TIFFOpen(fn_white, "w");
        if (!tif_out) {
            TIFFClose(tif_in);
            return 1;
        }

        // Set the output TIFF image dimensions and properties
        TIFFSetField(tif_out, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif_out, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif_out, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(tif_out, TIFFTAG_BITSPERSAMPLE, 16);
        TIFFSetField(tif_out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);

        TIFFSetField(tif_out, TIFFTAG_XRESOLUTION, xres);
        TIFFSetField(tif_out, TIFFTAG_YRESOLUTION, yres);
        TIFFSetField(tif_out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

        // Allocate memory for a scanline of input and output image
        uint16_t* scanline_in = (uint16_t*)_TIFFmalloc(TIFFScanlineSize(tif_in));
        uint16_t* scanline_out = (uint16_t*)_TIFFmalloc(TIFFScanlineSize(tif_out));

        // Process and write each scanline of input image
        for (uint32_t row = 0; row < height; row++) {
            TIFFReadScanline(tif_in, scanline_in, row);
            // Write the processed scanline to the output TIFF file
            TIFFWriteScanline(tif_out, scanline_in, row);
        }

        // Free the memory for the scanlines
        _TIFFfree(scanline_in);
        _TIFFfree(scanline_out);

        // Close the input and output TIFF files
        TIFFClose(tif_in);
        TIFFClose(tif_out);

        return true;


}

bool WzImageService::saveImageAsTiff(const WzImageBuffer& imageBuffer, const QString& filename) {
#ifdef Q_OS_WIN                          //定义在Windows中
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
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, imageBuffer.width);                        //图像宽
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, imageBuffer.height);                      //图像高
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, imageBuffer.bitDepth);                  //图像通道数
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);                        //数据压缩技术
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);                  //光度稀释
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, ""); // TODO 写点东西
        TIFFSetField(tif, TIFFTAG_MAKE, kTiffMake);                                      //扫描仪厂家名称
        TIFFSetField(tif, TIFFTAG_MODEL, ""); // TODO 写点东西， 本程序拍摄的图片写入这个字段
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, imageBuffer.samplesPerPixel);         //像素
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);                                      //每条数据的行数
        TIFFSetField(tif, TIFFTAG_XRESOLUTION, 600.0);                                   //像素/ x分辨率
        TIFFSetField(tif, TIFFTAG_YRESOLUTION, 600.0);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);                         //分辨率单位
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

    // 2022-03-29 11:25:20 by wz
    // fixbug-3
    if (imageBuffer.bitDepth == 8 && imageBuffer.samplesPerPixel == 1) {

    } else {
        #ifdef Q_OS_WIN
        std::wstring fn2(fn);
        #else
        std::string fn2(fn);
        #endif
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fn2);
        if (image.get()) {
            image->readMetadata();
            Exiv2::ExifData &exifData = image->exifData();
            #ifdef QT_DEBUG
            for (auto iter = exifData.begin(); iter != exifData.end(); iter++)
                qDebug() << QString::fromStdString(iter->key()) << QString::fromStdString(iter->value().toString());
            #endif
            exifData["Exif.Image.ExposureTime"] = Exiv2::Rational(imageBuffer.exposureMs, 1000);
            exifData["Exif.Photo.ExposureTime"] = Exiv2::Rational(imageBuffer.exposureMs, 1000);
            QString dateTimeStr = imageBuffer.captureDateTime.toString("yyyy:MM:dd hh:mm:ss");
            if (dateTimeStr != "") {
                exifData["Exif.Image.DateTimeOriginal"] = dateTimeStr.toStdString();
                exifData["Exif.Photo.DateTimeOriginal"] = dateTimeStr.toStdString();
            }
            image->setExifData(exifData);
            for (auto iter = exifData.begin(); iter != exifData.end(); iter++)
            {
                qDebug() << QString::fromStdString(iter->key()) << QString::fromStdString(iter->value().toString());
            }
            image->writeMetadata();
            qInfo() << "exiv2 saved";
        } else {
            qWarning() << "exiv2.open error";
        }
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
    emit lowChanged();
    emit highChanged();
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
    emit lowMarkerChanged();
    emit highMarkerChanged();
};

// TODO 改为异步
void WzImageService::calculateLowHigh() {
    int low, high;
    if (nullptr == m_imageBuffer || m_imageBuffer->bytesCountOfBuf == 0) {
        qWarning() << "calculateLowHigh, 图片缓冲区无内容";
        emit lowHighCalculated(WzEnum::NullBuffer, 0, 0);
        return;
    }
    m_imageBuffer->update();
    m_imageBuffer->getMinMax(low, high);
    m_low = low;
    m_high = high;
    emit lowChanged();
    emit highChanged();
    emit lowHighCalculated(WzEnum::Success, low, high);
}

void WzImageService::calculateLowHighMarker()
{
    int low, high;
    if (nullptr == m_markerBuffer || m_markerBuffer->bytesCountOfBuf == 0) {
        qWarning() << "calculateLowHighMarker, Marker 缓冲区无内容";
        emit lowHighMarkerCalculated(WzEnum::NullBuffer, 0, 0);
        return;
    }
    m_markerBuffer->update();
    m_markerBuffer->getMinMax(low, high);
    m_lowMarker = low;
    m_highMarker = high;
    emit lowMarkerChanged();
    emit highMarkerChanged();
    emit lowHighMarkerCalculated(WzEnum::Success, low, high);
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

QVariantMap WzImageService::getActiveImageInfo(const QJsonObject &params) {
    QVariantMap imageInfo;

    if (nullptr == m_imageBuffer)
        return imageInfo;

    // TODO 字段待补全
    imageInfo["imageFile"] = m_imageBuffer->imageFile;
    imageInfo["imageThumbFile"] = m_imageBuffer->imageThumbFile;
    imageInfo["openedLight"] = "";
    imageInfo["colorChannel"] = 0;

    QString imageWhiteFile = getMarkerFilename(m_imageBuffer->imageFile);
    if (QFileInfo::exists(imageWhiteFile))
        imageInfo["imageWhiteFile"] = imageWhiteFile;

    if (params.contains("generateImageInfo")) {
        int min, max;
        m_imageBuffer->getMinMax(min, max);
        imageInfo["grayLow"] = min;
        imageInfo["grayHigh"] = max;
        imageInfo["grayMin"] = min;
        imageInfo["grayMax"] = max;
        imageInfo["imageInvert"] = 0;
    }

    return imageInfo;
};

void WzImageService::deleteImage(QVariantMap imageInfo) {
    // TODO
    Q_UNUSED(imageInfo)
}

void WzImageService::closeActiveImage() {
    uint8_t* image8bit = new uint8_t[800*600];
    memset(image8bit, 0x7f, 800 * 600);
    g_imageView->updateImage(image8bit, 800, 600);
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

bool WzImageService::saveAsImage(const QJsonObject &params) {
    QString format = params["format"].toString();
    bool isSaveMarker = params["isSaveMarker"].toBool();
    QString fileName = params["fileName"].toString();
    if (format == "tiff16") {
        if (isSaveMarker && QFile::exists(m_markerImageName))//文件是否存在和是不是marker图
            copyTiff16Image(fileName, "_marker", m_markerImageName);
        if(saveImageAsTiff(*m_imageBuffer, fileName)){
            if(m_changedWhite){
                QImage m_image(fileName);
                return ImageFlipBlackWhite(m_image,fileName);
            }
            else
            {
                return true;
            }
        }
        return false;
    }
    if (format == "tiff8") {
        if (nullptr == m_image8bit)
            return false;
        WzImageBuffer imageBuffer8bit;
        imageBuffer8bit.width = m_imageBuffer->width;
        imageBuffer8bit.height = m_imageBuffer->height;
        imageBuffer8bit.exposureMs = params["exposureMs"].toInt();
        imageBuffer8bit.captureDateTime =
            QDateTime::fromString(params["captureDate"].toString(),
                                  "yyyy-MM-ddTHH:mm:ss.zzz");
        imageBuffer8bit.samplesPerPixel = 1;
        imageBuffer8bit.bitDepth = 8;
        imageBuffer8bit.buf = m_image8bit;

#ifdef MED_FIL_TIFF8
        if (params["isMedFilTiff8"].toBool()) {
            auto filSize = params["medFilSize"].toInt(9);
            if (filSize < 3)
                filSize = 3;
            else if (filSize % 2 == 0)
                filSize += 1;
            cv::Mat src(imageBuffer8bit.height, imageBuffer8bit.width, CV_8UC1, imageBuffer8bit.buf);
            cv::Mat dst;
            cv::medianBlur(src, dst, filSize);
            for (int row = 0; row < imageBuffer8bit.height; row++)
                for (int col = 0; col < imageBuffer8bit.width; col++) {
                    int idx = imageBuffer8bit.width * row + col;
                    imageBuffer8bit.buf[idx] = dst.at<uchar>(idx);
                }
        }
#endif

        return saveImageAsTiff(imageBuffer8bit, fileName);;
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

void WzImageService::saveChemiMarker(const QString &fileName, const QString &format)
{
    QUrl url(fileName);
    QFileInfo oldFile;
    if (url.isLocalFile())
        oldFile = url.toLocalFile();
    else
        oldFile = fileName;
    QString suffix;
    if (format.startsWith("tiff"))
        suffix = "tif";
    else if (format == "jpeg")
        suffix = "jpg";
    else if (format == "png")
        suffix = "png";

    QFileInfo fnChemi(oldFile.absolutePath(), oldFile.completeBaseName() + "_chemi." + suffix);
    QFileInfo fnMarker(oldFile.absolutePath(), oldFile.completeBaseName() + "_marker." + suffix);
    qDebug() << fnChemi.absoluteFilePath();
    qDebug() << fnMarker.absoluteFilePath();

    auto saveTiff = [=](uint8_t* buf, int samplesPerPixel, QString fn) {
        if (buf == nullptr) {
            qInfo() << "saveChemiMarker, buf == nullptr, fn:" << fn;
            return false;
        }
        WzImageBuffer imageBuffer8bit;
        imageBuffer8bit.width = m_imageBuffer->width;
        imageBuffer8bit.height = m_imageBuffer->height;
        imageBuffer8bit.bitDepth = 8;
        imageBuffer8bit.samplesPerPixel = samplesPerPixel;
        imageBuffer8bit.buf = buf;
        return saveImageAsTiff(imageBuffer8bit, fn);
    };

    auto bit8ToRgbImage = [=](uint8_t* buf) {
        QImage img(m_imageBuffer->width, m_imageBuffer->height, QImage::Format_ARGB32);
        img.fill(Qt::gray);

        uint8_t* pImageData = buf;
        for (uint32_t row = 0; row < m_imageBuffer->height; row++) {
            uchar* line = img.scanLine(row);
            for (uint32_t col = 0; col < m_imageBuffer->width; col++) {
                *line = *pImageData;
                line++;
                *line = *pImageData;
                line++;
                *line = *pImageData;
                line++;
                *line = 0xff;
                line++;
                pImageData++;
            }
        }
        return img;
    };

    if ("tiff8" == format) {
        saveTiff(m_imageMarker8bit, 1, fnMarker.absoluteFilePath());
        saveTiff(m_imageChemi8bit, 1, fnChemi.absoluteFilePath());
    } else if ("tiff24" == format) {
        auto bit24Chemi = WzUtils::bit8BufExpand24(m_imageChemi8bit, m_imageBuffer->width, m_imageBuffer->height);
        saveTiff(bit24Chemi, 3, fnChemi.absoluteFilePath());
        delete [] bit24Chemi;
        auto bit24Marker = WzUtils::bit8BufExpand24(m_imageMarker8bit, m_imageBuffer->width, m_imageBuffer->height);
        saveTiff(bit24Marker, 3, fnMarker.absoluteFilePath());
        delete [] bit24Marker;
    } else if ("jpeg" == format) {
        auto chemiImage = bit8ToRgbImage(m_imageChemi8bit);
        auto markerImage = bit8ToRgbImage(m_imageMarker8bit);
        chemiImage.save(fnChemi.absoluteFilePath(), "jpg", 100);
        markerImage.save(fnMarker.absoluteFilePath(), "jpg", 100);
    } else if ("png" == format) {
        auto chemiImage = bit8ToRgbImage(m_imageChemi8bit);
        auto markerImage = bit8ToRgbImage(m_imageMarker8bit);
        chemiImage.save(fnChemi.absoluteFilePath(), "png", 100);
        markerImage.save(fnMarker.absoluteFilePath(), "png", 100);
    } else {
        qInfo() << "saveChemiMarker, format == tiff16, no save";
    }
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
    if (m_imageView == nullptr)
        m_imageView = g_imageView;

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

    qInfo() << "m_overExposureHint:" << m_overExposureHint;

    if (!showMarker) {
        if (m_invert)
            // 这里两个if支里的代码唯一的区别就是有没有启用过爆提示
            if (m_overExposureHint) {
                for(int i=0; i<pixelCount; i++) {
                    uint8_t gray = 255 - static_cast<char>(m_grayTable[pWord[i]]);
                    uint8_t r = m_colorTableR[gray];
                    uint8_t g = m_colorTableG[gray];
                    uint8_t b = m_colorTableB[gray];
                    if (pWord[i] >= m_overExposureHintValue) {
                        int rr, gg, bb;
                        m_overExposureHintColor.getRgb(&rr, &gg, &bb);
                        m_image32bit[i] = (0xff << 24) | (rr << 16) | (gg << 8) | bb;
                    } else
                        m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                    m_image8bit[i] = gray;
                }
            } else {
                for(int i=0; i<pixelCount; i++) {
                    uint8_t gray = 255 - static_cast<char>(m_grayTable[pWord[i]]);
                    uint8_t r = m_colorTableR[gray];
                    uint8_t g = m_colorTableG[gray];
                    uint8_t b = m_colorTableB[gray];
                    m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                    m_image8bit[i] = gray;
                }
            }
        else
            // 这里两个if支里的代码唯一的区别就是有没有启用过爆提示
            if (!m_overExposureHint) {
                for(int i=0; i<pixelCount; i++) {
                    uint8_t gray = static_cast<char>(m_grayTable[pWord[i]]);
                    uint8_t r = m_colorTableR[gray];
                    uint8_t g = m_colorTableG[gray];
                    uint8_t b = m_colorTableB[gray];
                    m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                    m_image8bit[i] = gray;
                }
            } else {
                for(int i=0; i<pixelCount; i++) {
                    uint8_t gray = static_cast<char>(m_grayTable[pWord[i]]);
                    uint8_t r = m_colorTableR[gray];
                    uint8_t g = m_colorTableG[gray];
                    uint8_t b = m_colorTableB[gray];
                    if (pWord[i] >= m_overExposureHintValue) {
                        int rr, gg, bb;
                        m_overExposureHintColor.getRgb(&rr, &gg, &bb);
                        m_image32bit[i] = (0xff << 24) | (rr << 16) | (gg << 8) | bb;
                    } else
                        m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                    m_image8bit[i] = gray;
                }
            }
    } else {
        if (m_showChemi) {
            if (nullptr != m_markerBuffer->buf) {
                uint16_t* markerBufPtr = reinterpret_cast<uint16_t*>(m_markerBuffer->buf);
                // 使用了伪彩调色板
                if (m_isPseudoColor) {
                    // 化学发光叠加了白光图/使用伪彩
                    // 这两个if分支里的代码唯一的区别就是有没有启用过曝提示
                    if (m_overExposureHint) {
                        for(int i=0; i<pixelCount; i++) {
                            int gray = m_grayTable[pWord[i]];
                            int alphaLevel = gray + 1;
                            int markerGray = m_grayTableMarker[markerBufPtr[i]] * (256 - alphaLevel);
                            uint8_t r = (m_colorTableR[255 - gray] * alphaLevel + markerGray) >> 8;
                            uint8_t g = (m_colorTableG[255 - gray] * alphaLevel + markerGray) >> 8;
                            uint8_t b = (m_colorTableB[255 - gray] * alphaLevel + markerGray) >> 8;
                            if (pWord[i] > m_overExposureHintValue) {
                                int rr, gg, bb;
                                m_overExposureHintColor.getRgb(&rr, &gg, &bb);
                                m_image32bit[i] = (0xff << 24) | (rr << 16) | (gg << 8) | bb;
                            } else
                                m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                            m_image8bit[i] = gray;
                            m_imageChemi8bit[i] = 255 - gray;
                            m_imageMarker8bit[i] = m_grayTableMarker[markerBufPtr[i]];
                        }
                    } else {
                        for(int i=0; i<pixelCount; i++) {
                            int gray = m_grayTable[pWord[i]];
                            int alphaLevel = gray + 1;
                            int markerGray = m_grayTableMarker[markerBufPtr[i]] * (256 - alphaLevel);
                            uint8_t r = (m_colorTableR[255 - gray] * alphaLevel + markerGray) >> 8;
                            uint8_t g = (m_colorTableG[255 - gray] * alphaLevel + markerGray) >> 8;
                            uint8_t b = (m_colorTableB[255 - gray] * alphaLevel + markerGray) >> 8;
                            m_image32bit[i] = (0xff << 24) | (r << 16) | (g << 8) | b;
                            m_image8bit[i] = gray;
                            m_imageChemi8bit[i] = 255 - gray;
                            m_imageMarker8bit[i] = m_grayTableMarker[markerBufPtr[i]];
                        }
                    }
                } else {
                    // 化学发光叠加了白光图/没有使用伪彩
                    // 这两个if分支里的代码唯一的区别就是有没有启用过曝提示
                    if (m_overExposureHint) {
                        for(int i=0; i<pixelCount; i++) {
                            int gray;
                            if (pWord[i] < m_low)
                                gray = m_grayTableMarker[markerBufPtr[i]];
                            else
                                gray = m_grayTableMarker[markerBufPtr[i]] - m_grayTable[pWord[i]];
                            if (gray < 0) gray = 0;
                            if (pWord[i] >= m_overExposureHintValue) {
                                int rr, gg, bb;
                                m_overExposureHintColor.getRgb(&rr, &gg, &bb);
                                m_image32bit[i] = (0xff << 24) | (rr << 16) | (gg << 8) | bb;
                            } else
                                m_image32bit[i] = (0xff << 24) | (gray << 16) | (gray << 8) | gray;
                            m_image8bit[i] = gray;
                            m_imageChemi8bit[i] = 255 - m_grayTable[pWord[i]];
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
    if (imageBuffer.buf && m_imageBuffer->buf)
        memcpy(m_imageBuffer->buf, imageBuffer.buf, imageBuffer.bytesCountOfBuf);
    m_imageBuffer->width = imageBuffer.width;
    m_imageBuffer->height = imageBuffer.height;
    m_imageBuffer->bitDepth = imageBuffer.bitDepth;

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
        for (int i = 0; i < realThumbHeight * realThumbWidth; i++) {
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

void WzImageService::removePlateBorder(WzImageBuffer &imageBuffer)
{

    QRect rect(0, 0, 0, 0);
    //int circleBorderWidth = 0;
    if (imageBuffer.width == 0)
        return;
    if (imageBuffer.height == 0)
        return;

    uint32_t *rowMeans = new uint32_t[imageBuffer.height];
    int32_t *rowSlope = new int32_t[imageBuffer.height];
    int32_t maxSlope = 0, minSlope = 0, meanSlope = 0, totalSlope = 0;

    // 计算每行的平均值
    for(uint row = 0; row < imageBuffer.height; row++) {
        uint64_t row_total = 0;
        for (uint col = 0; col < imageBuffer.width; col++) {
            row_total += imageBuffer.bit16Array[col + row * imageBuffer.width];
        }
        rowMeans[row] = row_total / imageBuffer.width;
    }

    // 计算每行之间平均值的斜率和平均斜率
    rowSlope[0] = 0;
    for(uint row = 1; row < imageBuffer.height; row++) {
        rowSlope[row] = rowMeans[row] - rowMeans[row - 1];
        totalSlope += qAbs(rowSlope[row]);
    }
    meanSlope = totalSlope / imageBuffer.height;

    // 从图片的上方往下方寻找第一个较大的斜率, 该斜率会首先大幅上升, 然后再大幅跌落到负值.
    for (uint row = 1; row < imageBuffer.height; row++) {
        if (maxSlope < rowSlope[row]) {
            maxSlope = rowSlope[row];
        };
        if (minSlope > rowSlope[row]) {
            minSlope = rowSlope[row];
            rect.setTop(row);
        };

        if (maxSlope > meanSlope * 10 && minSlope < meanSlope * -10 &&
                rowMeans[row] > 0) {
            break;
        }

    }
    if (rect.top() == 0) {
        delete [] rowSlope;
        delete [] rowMeans;
        return;
    }
    // end //

    // 从图片的下方往上方寻找第一个较大的斜率, 该斜率会首先大幅下跌, 然后再大幅上升到正值.
    minSlope = 0;
    maxSlope = 0;
    for (int row = imageBuffer.height - 1; row >= 0; row--) {
        if (maxSlope < rowSlope[row]) {
            maxSlope = rowSlope[row];
            rect.setBottom(row);
        };
        if (minSlope > rowSlope[row]) {
            minSlope = rowSlope[row];
        };

        if (maxSlope > meanSlope * 10 && minSlope < meanSlope * -10 &&
                rowMeans[row] > 0) {
            break;
        }

    }
    if (rect.bottom() == 0) {
        delete [] rowSlope;
        delete [] rowMeans;
        return;
    }
    delete [] rowSlope;
    delete [] rowMeans;

    if (rect.bottom() == 0)
        return;

    // 获取每列的平均值, 然后再计算斜率, 最大和最小的两个斜率基本上是培养皿的左右边缘
    uint32_t *colMeans = new uint32_t[imageBuffer.width];
    int32_t *colSlope = new int32_t[imageBuffer.width];

    for(uint col = 0; col < imageBuffer.width; col++) {
        uint64_t col_total = 0;
        for (uint row = 0; row < imageBuffer.height; row++) {
            col_total += imageBuffer.bit16Array[col + row * imageBuffer.width];
        }
        colMeans[col] = col_total / imageBuffer.height;
    }

    colSlope[0] = 0;
    totalSlope = 0;
    for(uint col = 1; col < imageBuffer.width; col++) {
        colSlope[col] = colMeans[col] - colMeans[col - 1];
        totalSlope += qAbs(colSlope[col]);
    }
    meanSlope = totalSlope / imageBuffer.width;

    maxSlope = 0;
    minSlope = 0;
    for (uint col = 1; col < imageBuffer.width; col++) {
        if (maxSlope < colSlope[col]) {
            maxSlope = colSlope[col];
        };
        if (minSlope > colSlope[col]) {
            minSlope = colSlope[col];
            rect.setLeft(col);
        };

        if (maxSlope > meanSlope * 10 && minSlope < meanSlope * -10 &&
                colMeans[col] > 0) {
            break;
        }

    }
    if (rect.left() == 0) {
        delete [] colSlope;
        delete [] colMeans;
        return;
    }

    minSlope = 0;
    maxSlope = 0;
    for (int col = imageBuffer.width - 1; col >= 0; col--) {
        if (maxSlope < colSlope[col]) {
            maxSlope = colSlope[col];
            rect.setRight(col);
        };
        if (minSlope > colSlope[col]) {
            minSlope = colSlope[col];
        };

        if (maxSlope > meanSlope * 10 && minSlope < meanSlope * -10 &&
                colMeans[col] > 0) {
            break;
        }

    }
    if (rect.right() == 0) {
        delete [] colSlope;
        delete [] colMeans;
        return;
    }
    delete [] colSlope;
    delete [] colMeans;

    if (rect.right() == 0)
        return;

    rect.adjust(3, 3, -3, -3);

    // 获得圆圈内的平均值
    QPainterPath pp;
    pp.addEllipse(rect);
    uint64_t pixelCount = 0;
    uint64_t totalGrayValues = 0;
    for (uint row = 0; row < imageBuffer.height; row++) {
        for (uint col = 0; col < imageBuffer.width; col++) {
            if (pp.contains(QPoint(col, row))) {
                pixelCount++;
                totalGrayValues += imageBuffer.bit16Array[col + row * imageBuffer.width];
                //imageBuffer.bit16Array[col + row * imageBuffer.width] = backgroundValue;
            }
        }
    }
    int backgroundValue = totalGrayValues / pixelCount;
    for (uint row = 0; row < imageBuffer.height; row++) {
        for (uint col = 0; col < imageBuffer.width; col++) {
            if (!pp.contains(QPoint(col, row))) {
                imageBuffer.bit16Array[col + row * imageBuffer.width] = backgroundValue;
            }
        }
    }
}

void WzImageService::copyTiff16Image(const QString &destFileName,
                                     const QString &suffix,
                                     const QString &srcFileName)
{
    // 提取目标文件名中的路径和不带后缀的文件名, 将这个文件名加上 _16bit 的后缀作为新文件名
    QUrl url(destFileName);
    QFileInfo fi;
    if (url.isLocalFile())
        fi = url.toLocalFile();
    else
        fi = destFileName;
    QString newFileName = fi.completeBaseName() + suffix + ".tif";
    QFileInfo fiNew(fi.absoluteDir(), newFileName);
    qDebug() << QString("Src: %1, Dest: %2").arg(srcFileName, fiNew.absoluteFilePath());
    QFile::copy(srcFileName, fiNew.absoluteFilePath());
}

QString WzImageService::getMarkerFilename(const QString &fileName)
{
    // E:\Users\SHST\Desktop\2021-11-04\2021-11-04_17.57.35_1.tif

    qInfo() << "WzImageService::getMarkerFilename";
    qInfo() << "\t" << fileName;

    QFileInfo fi(fileName);
    qInfo() << "\t" << fi.fileName();

    QRegularExpression re("\\d+-\\d+-\\d+_\\d+.\\d+.\\d+");
    auto match = re.match(fi.fileName());

    if (match.hasMatch()) {
        auto dt = QDateTime::fromString(match.captured(0), "yyyy-MM-dd_hh.mm.ss");
        auto dtStr = dt.toString("yyyy-MM-dd_hh-mm-ss");
        QString markerFilename = "marker_" + dtStr + ".tif";

        QFileInfo fi(fileName);
        QDir dirMarker(fi.absoluteDir());
        if (!dirMarker.cd(kMarkerFolder)) {
            qInfo() << "\t can't cd" << kMarkerFolder;
            return "";
        };
        QFileInfo fiMarker(dirMarker, markerFilename);

        qInfo() << "\t" << fiMarker.absoluteFilePath();

        return fiMarker.absoluteFilePath();
    } else {
        qInfo() << "\t no match";
    }
    return "";
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
    emit lowChanged();
}

void WzImageService::setHigh(int high)
{
    m_high = high;
    emit highChanged();
}

void WzImageService::setLowMarker(int lowMarker)
{
    m_lowMarker = lowMarker;
    emit lowMarkerChanged();
}

void WzImageService::setHighMarker(int highMarker)
{
    m_highMarker = highMarker;
    emit highMarkerChanged();
}

void WzImageService::setChangedWhite(bool changedWhite)
{
    m_changedWhite=changedWhite;
    emit ChangedWhiteChanged();
}

bool WzImageService::getChangedWhite() const
{
    return m_changedWhite;
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

    g_imageView->updateImage(imageData, imageWidth, imageHeight);
    delete [] imageData;
}

bool WzImageService::getIsColorChannel() const
{
    return m_isColorChannel;
}

void WzImageService::setIsColorChannel(bool isColorChannel)
{
    m_isColorChannel = isColorChannel;
    emit isColorChannelChanged();
}

void WzImageService::updateColorTable()
{
    if (g_imageView) {
        QList<QVariant> list = g_imageView->getColorTable().toList();
        int m = list.count();
        if (m > 256) m = 256;
        for (int n = 0; n < m; n++) {
            QMap<QString, QVariant> map = list[n].toMap();
            m_colorTableR[n] = map["R"].toInt();
            m_colorTableG[n] = map["G"].toInt();
            m_colorTableB[n] = map["B"].toInt();
        }
    } else {
        for (int i = 0; i < 256; i++) {
            m_colorTableR[i] = i;
            m_colorTableG[i] = i;
            m_colorTableB[i] = i;
        }
    }
}

bool WzImageService::getIsPseudoColorBarInImage() const
{
    return m_isPseudoColorBarInImage;
}

void WzImageService::setIsPseudoColorBarInImage(bool isPseudoColorBarInImage)
{
    m_isPseudoColorBarInImage = isPseudoColorBarInImage;
    emit isPseudoColorBarInImageChanged();
}

bool WzImageService::getIsPseudoColor() const
{
    return m_isPseudoColor;
}

void WzImageService::setIsPseudoColor(bool isPseudoColor)
{
    m_isPseudoColor = isPseudoColor;
    emit isPseudoColorChanged();
}

void WzImageService::drawColorBar(QImage &image)
{
    if (!g_imageView)
        return;
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
    uint8_t *buffer24bit = new uint8_t[m_imageBuffer->width * m_imageBuffer->height * 3];
    uint8_t *buffer24bitPtr = buffer24bit;
    for (uint32_t row = 0; row < m_imageBuffer->height; row++) {
        for (uint32_t col = 0; col < m_imageBuffer->width; col++) {
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
    imageBuffer24bit.width = m_imageBuffer->width;
    imageBuffer24bit.height = m_imageBuffer->height;
    imageBuffer24bit.bitDepth = 8;
    imageBuffer24bit.samplesPerPixel = 3;
    imageBuffer24bit.buf = buffer24bit;
    bool ret = saveImageAsTiff(imageBuffer24bit, fileName);
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

bool WzImageService::getOverExposureHint() const
{
    return m_overExposureHint;
}

void WzImageService::setOverExposureHint(bool overExposureHint)
{
    qInfo() << "overExposureHint:" << overExposureHint;
    if (m_overExposureHint == overExposureHint)
        return;
    m_overExposureHint = overExposureHint;
    emit overExposureHintChanged();
    updateView();
}

int WzImageService::getOverExposureHintValue() const
{
    return m_overExposureHintValue;
}

void WzImageService::setOverExposureHintValue(int overExposureHintValue)
{
    qInfo() << "overExposureHintValue:" << overExposureHintValue;
    if (m_overExposureHintValue == overExposureHintValue)
        return;
    m_overExposureHintValue = overExposureHintValue;
    emit overExposureHintValueChanged();
    updateView();
}

QColor WzImageService::getOverExposureHintColor() const
{
    return m_overExposureHintColor;
}

void WzImageService::setOverExposureHintColor(const QColor &overExposureHintColor)
{
    if (m_overExposureHintColor == overExposureHintColor)
        return;
    m_overExposureHintColor = overExposureHintColor;
    emit overExposureHintColorChanged();
    updateView();
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
