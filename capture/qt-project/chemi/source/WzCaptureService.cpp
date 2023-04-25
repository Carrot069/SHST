#include "WzCaptureService.h"

WzCaptureService::WzCaptureService(QObject *parent):
    QObject(parent),
#ifdef WIN32
    #ifdef DEMO
    m_camera(new WzTestCamera(parent)),
    #endif
    #ifdef GEL_CAPTURE
    m_camera(new WzKsjCamera(parent)),
    #endif
    #ifdef PVCAM
    m_camera(dynamic_cast<WzAbstractCamera*>(new WzPvCamera(parent))),
    #endif
    #ifdef ATIK
    m_camera(dynamic_cast<WzAbstractCamera*>(new WzAtikCamera(parent))),
    #endif
#endif
#ifdef MAC
    m_camera(new WzTestCamera(parent)),
#endif
    m_previewImageData(nullptr),
    m_previewImageSize(new QSize(100, 100)),
    m_captureTimer(new QTimer())
{
    qDebug() << "WzCaptureService";

    readCameraSNList();

    QObject::connect(m_camera, SIGNAL(previewImageUpdated()), this, SLOT(previewImageUpdated()));
    QObject::connect(m_camera, SIGNAL(cameraState(WzCameraState::CameraState)), this, SLOT(cameraStateUpdated(WzCameraState::CameraState)));
    QObject::connect(m_captureTimer, SIGNAL(timeout()), this, SLOT(captureTimer()));

    m_captureTimer->setSingleShot(false);
    m_captureTimer->setInterval(900);

    m_camera->setCameraCallback(this);
#ifdef Second_Chemi
    m_isSecondChemi=false;
#else
    m_isSecondChemi=true;
#endif

    connect(&m_autoFocus, &WzAutoFocus::focusFar, this, &WzCaptureService::autoFocusFar);
    connect(&m_autoFocus, &WzAutoFocus::focusNear, this, &WzCaptureService::autoFocusNear);
    connect(&m_autoFocus, &WzAutoFocus::focusStop, this, &WzCaptureService::autoFocusStop);
    connect(&m_autoFocus, &WzAutoFocus::getImage, this, &WzCaptureService::autoFocusGetImage);
    connect(&m_autoFocus, &WzAutoFocus::log, this, &WzCaptureService::autoFocusLog);
    connect(&m_autoFocus, &WzAutoFocus::finished, this, &WzCaptureService::autoFocusFinished);
    connect(this, &WzCaptureService::SendStorageWhite, m_camera, &WzAbstractCamera::SetstorageWhite);
}

WzCaptureService::~WzCaptureService()
{
    m_captureTimer->stop();
    delete m_captureTimer;
    delete m_previewImageSize;
    if (m_previewImageData) {
        delete[] m_previewImageData;
        m_previewImageData = nullptr;
    }    
    delete m_camera;
}

bool WzCaptureService::enableCameraPreview(bool enabled, int binning, bool isSlow) {
    m_camera->setParam("IsSlowPreview", isSlow);
    m_camera->setParam("Binning", binning);
    qDebug() << "enableCameraPreview, enabled = " << enabled << ", binning = " << binning;
    return m_camera->setPreviewEnabled(enabled);
}

bool WzCaptureService::resetPreview()
{
    m_camera->resetPreview();
    return true;
}

bool WzCaptureService::capture(QVariantMap params) {
    m_captureParams = params;

    qDebug() << "WzCaptureService::capture, params:" << params;
    m_captureId = 0;
    m_captureCount = 1;
    m_captureIndex = 0;
    emit this->captureState(Init);
    m_camera->setParam("ImagePath", m_imagePath);
    m_camera->setParam("Binning", params["binning"]);
    m_camera->setParam("AE", m_isAutoExposure);
    m_camera->setParam("SampleName", params["sampleName"]);
    m_camera->setParam("grayAccumulate", params["grayAccumulate"]);
    m_camera->setParam("removeFluorCircle", params["removeFluorCircle"]);

    QString lt = "";
    if (params.contains("openedLightType"))
        lt = params["openedLightType"].toString();
    if (params["isLightOpened"].toBool() &&
            (lt == "uv_penetrate" || lt == "uv_penetrate_force" ||
             lt == "white_down" || lt == "red" || lt == "green" || lt == "blue" ||
             lt == "uv_reflex1" || lt == "uv_reflex2")) {
        m_camera->setParam("isThumbNegative", false);
    } else {
        m_camera->setParam("isThumbNegative", true);
    }

    bool ret = m_camera->capture(qMax(1, params["exposureMs"].toInt()));
    m_captureTimer->start();
    return ret;
}

bool WzCaptureService::captureMulti(QVariantMap params) {
    qDebug() << "WzCaptureService::captureMulti, params:" << params;
    m_captureParams = params;

    emit this->captureState(Init);

    m_captureId = QDateTime::currentSecsSinceEpoch();
    m_camera->setParam("ImagePath", m_imagePath);
    m_camera->setParam("Binning", params["binning"].toInt());
    m_camera->setParam("AE", m_isAutoExposure);
    m_camera->setParam("SampleName", params["sampleName"]);
    m_camera->setParam("removeFluorCircle", params["removeFluorCircle"]);

    QList<int> exposureMsList;
    if (params["multiType"] == "gray") {
        m_captureCount = params["grayAccumulate_frameCount"].toInt();
        m_camera->setParam("grayAccumulate", true);
        m_camera->setParam("grayAccumulateAddExposure", params["grayAccumulateAddExposure"]);
        for (int i = 0; i < m_captureCount; i++)
            exposureMsList.append(qMax(1, params["exposureMs"].toInt()));
    } else if (params["multiType"] == "time") {
        m_captureCount = params["timeAccumulate_frameCount"].toInt();
        m_camera->setParam("grayAccumulate", false);
        // 在基础时间上累加时间
        int exposureMs = qMax(1, params["exposureMs"].toInt());
        int exposureMsStep = params["timeAccumulate_exposureMs"].toInt();
        for (int i = 0; i <= m_captureCount; i++)
            exposureMsList.append(exposureMs + exposureMsStep * i);
        // 除了时间累积设置的帧数还包括基础时间的一帧，也就是说最少拍2帧
        m_captureCount++;
    } else if (params["multiType"] == "time_list") {
        m_camera->setParam("grayAccumulate", false);
        foreach(QVariant v, params["timeSequenceExposureMsList"].toList()) {
            exposureMsList << qMax(1, v.toInt());
        }
        m_captureCount = exposureMsList.count();
    } else {
        qWarning("无效的多帧拍摄参数");
        return false;
    }

    QString lt = "";
    if (params.contains("openedLightType"))
        lt = params["openedLightType"].toString();
    if (params["isLightOpened"].toBool() &&
            (lt == "uv_penetrate" || lt == "uv_penetrate_force" ||
             lt == "uv_reflex1" || lt == "uv_reflex2" ||
             lt == "white_down" || lt == "red" || lt == "green" || lt == "blue")) {
        m_camera->setParam("isThumbNegative", false);
    } else {
        m_camera->setParam("isThumbNegative", true);
    }

    qDebug() << "exposureMsList:" << exposureMsList;
    int* exposureMsArray = new int[exposureMsList.count()];
    for (int i = 0; i < exposureMsList.count(); i++) {
        exposureMsArray[i] = exposureMsList[i];
    }
    m_captureTimer->start();
    int ret = m_camera->capture(exposureMsArray, exposureMsList.count());
    delete []exposureMsArray;
    if (ret != ERROR_NONE)
        return false;
    return true;
}

void WzCaptureService::connectCamera() {
    m_camera->connect();
}

void WzCaptureService::disconnectCamera() {
    m_camera->disconnect();
}

void WzCaptureService::previewImageUpdated() {
    m_previewCount++;
    if (m_previewCount % 20 == 0)
        qDebug("WzCaptureService::previewImageUpdated, %lld", m_previewCount);

    QSize* imageSize = m_camera->getImageSize();

    {
        QMutexLocker lock(&m_previewImageDataLock);
        if (imageSize->width() != m_previewImageSize->width() ||
                imageSize->height() != m_previewImageSize->height()) {
            if (nullptr != m_previewImageData) {
                delete[] m_previewImageData;
                m_previewImageData = nullptr;
            }
            m_previewImageSize->setWidth(imageSize->width());
            m_previewImageSize->setHeight(imageSize->height());
        }

        if (nullptr == m_previewImageData) {
            m_previewImageData = new uchar[imageSize->width() * imageSize->height()];
        }

        m_camera->getImage(m_previewImageData);
        //std::ofstream outFile("D:/image.raw", std::ios::out | std::ios::binary);
        //outFile.write((char*)m_previewImageData, imageSize->width() * imageSize->height());
        //outFile.close();
    }

    m_markerImageSize.setWidth(imageSize->width());
    m_markerImageSize.setHeight(imageSize->height());
    /*
    if (!m_autoGotMarker) {
        if (hasMarkerImage())
            m_autoGotMarker = true;
        else
            getMarkerImage();
    }
    */

    if (g_liveImageView) {
        g_liveImageView->updateImage(m_previewImageData, *imageSize);
    }

    if (m_previewCount % 20 == 0)
        qDebug("WzCaptureService::previewImageUpdated, exit");
}

void WzCaptureService::captureTimer() {
    if (m_cameraState == WzCameraState::Exposure) {
        // 相机不支持异步拍摄
        if (m_camera->getLeftExposureMs() == -1) {
            m_exposurePercent = -1;
        } else {
            m_captureIndex = m_camera->getCurrentFrame();
            m_capturedCount = m_camera->getCapturedCount();
            int exposureMilliseconds = m_camera->getExposureMs();
            int percent = (exposureMilliseconds - m_camera->getLeftExposureMs()) * 100 / exposureMilliseconds;
            if (percent < 0)
                percent = 0;
            else if (percent > 100)
                percent = 100;
            m_exposurePercent = percent;
            m_leftExposureTime = WzUtils::getTimeStr(m_camera->getLeftExposureMs(), false);
            m_elapsedExposureTime = WzUtils::getTimeStr(exposureMilliseconds - m_camera->getLeftExposureMs(), false);
        }
        emit captureState(WzCaptureService::Exposure);
    }
}

void WzCaptureService::cameraStateUpdated(const WzCameraState::CameraState& state) {
    m_cameraState = state;
    switch(m_cameraState) {
    case WzCameraState::None:
        break;
    case WzCameraState::Connecting:
        break;
    case WzCameraState::Connected:
        m_cameraConnected = true;
        break;
    case WzCameraState::PreviewStarting:
    case WzCameraState::PreviewStarted:
    case WzCameraState::PreviewStopping:
    case WzCameraState::PreviewStopped:
        break;
    case WzCameraState::AutoExposure:
        emit captureState(WzCaptureService::AutoExposure);
        break;
    case WzCameraState::AutoExposureFinished:
        emit captureState(WzCaptureService::AutoExposureFinished);
        break;
    case WzCameraState::CaptureInit:
        emit captureState(WzCaptureService::Init);
        break;
    case WzCameraState::CaptureAborting:
        break;
    case WzCameraState::CaptureAborted:
        m_captureTimer->stop();
        emit captureState(WzCaptureService::Aborted);
        break;
    case WzCameraState::Exposure: {
        captureTimer();
        break;
    }
    case WzCameraState::Image:
        // TODO
        //m_camera->getLatestImageFile();
        m_captureIndex = m_camera->getCurrentFrame();
        emit captureState(WzCaptureService::Image);
        break;
    case WzCameraState::CaptureFinished: {
        m_captureIndex = m_camera->getCurrentFrame();
        m_capturedCount = m_camera->getCapturedCount();
        m_latestImageFile = m_camera->getLatestImageFile();
        m_latestThumbFile = m_camera->getLatestThumbFile();
        QVariant sampleName;
        m_camera->getParam("SampleName", sampleName);
        QVariant lowMarker, highMarker;
        m_camera->getParam("MarkerMinGray", lowMarker);
        m_camera->getParam("MarkerMaxGray", highMarker);
        QVariant captureDateTime;
        m_camera->getParam("captureDateTime", captureDateTime);
        WzDatabaseService db;
        QVariantMap imageInfo;
        imageInfo["imageFile"] = m_latestImageFile;
        imageInfo["imageThumbFile"] = m_latestThumbFile;
        imageInfo["imageSource"] = WzEnum::ImageSource::Capture;
        imageInfo["exposureMs"] = m_camera->getExposureMs();
        imageInfo["groupId"] = m_captureId;
        imageInfo["captureDate"] = captureDateTime.toDateTime();
        imageInfo["sampleName"] = sampleName.toString();
        imageInfo["grayLowMarker"] = lowMarker.toInt();
        imageInfo["grayHighMarker"] = highMarker.toInt();

        QVariant grayAccumulate, grayAccumulateAddExposure;
        m_camera->getParam("grayAccumulate", grayAccumulate);
        m_camera->getParam("grayAccumulateAddExposure", grayAccumulateAddExposure);
        if (grayAccumulate == true && grayAccumulateAddExposure == true) {
            imageInfo["exposureMs"] = m_camera->getExposureMs() * (m_captureIndex + 1);
        }

        QVariant isThumbNegative = true;
        m_camera->getParam("isThumbNegative", isThumbNegative);

        bool isSaveMarker = true;

        if (m_captureParams.contains("openedLightType") && m_captureParams.contains("isLightOpened")) {
            if (m_captureParams["isLightOpened"] == true) {
                imageInfo["openedLight"] = m_captureParams["openedLightType"];
                QString lt = m_captureParams["openedLightType"].toString();
                if (lt == "uv_penetrate" || lt == "uv_penetrate_force") {
                    imageInfo["imageInvert"] = 0x02;
                    isSaveMarker = false;
                } else if (lt == "uv_reflex1" || lt == "uv_reflex2") {
                    imageInfo["imageInvert"] = 0x02;
                    isSaveMarker = true;
                } else if (imageInfo["openedLight"] == "white_down") {
                    imageInfo["imageInvert"] = 0x04;
                    isSaveMarker = false;
                } else {
                    imageInfo["imageInvert"] = 0x01;
                    //if (lt == "red")
                        //imageInfo["palette"] = "SYPRO Ruby";
                        //imageInfo["colorChannel"] = 1;
                    //else if (lt == "green")
                        //imageInfo["palette"] = "SYBR Gree";
                        //imageInfo["colorChannel"] = 2;
                    //else if (lt == "blue")
                        //imageInfo["palette"] = "Stain Free";
                        //imageInfo["colorChannel"] = 3;
                }
            } else {
                imageInfo["openedLight"] = "";
                imageInfo["imageInvert"] = 0x01;
            }
        } else {
            imageInfo["openedLight"] = "";
            imageInfo["imageInvert"] = 0x01;
        }

        if (isSaveMarker)
            imageInfo["imageWhiteFile"] = saveMarkerImage(captureDateTime.toDateTime());

        db.saveImage(imageInfo);

        if (m_capturedCount == m_captureCount) {
            m_captureTimer->stop();
        }
        emit captureState(WzCaptureService::Finished);
        break;
    }
    case WzCameraState::Disconnecting:
    case WzCameraState::Disconnected:
        m_cameraConnected = false;
        break;
    case WzCameraState::CameraNotFound:
        // 所有种类的相机都没有时才向上层逻辑发送消息
        // 目前就一种相机，所以暂时不作处理
        break;
    case WzCameraState::Error:
        break;
    }
    emit this->cameraStateChanged(state);
}

void WzCaptureService::autoFocusGetImage()
{
    QMutexLocker lock(&m_previewImageDataLock);
    m_autoFocus.addFrame(m_previewImageData, *m_previewImageSize, g_liveImageView->getSelectedRect());
}

void WzCaptureService::autoFocusLog(const QString &msg)
{
    qDebug() << "AF:" << msg;
};

bool WzCaptureService::cameraSN(const QString sn) {
#ifdef HARDLOCK
    if (m_cameraSNList.contains(sn))
        return true;
    else {
        qInfo() << "未许可的序列号: " << sn;
        return false;
    }
#else
    return true;
#endif
}

void WzCaptureService::abortCapture() {
    m_camera->abortCapture();
}

int WzCaptureService::getCameraCount() {
    if (m_camera) {
        return 1; // TODO 2021-4-4 暂时硬编码
    }
    return 0;
}

void WzCaptureService::setPreviewExposureMs(int exposureMs) {
    m_camera->setParam("ExposureMs", exposureMs);
}

bool WzCaptureService::getMarkerImage()
{
    QMutexLocker lock(&m_markerTiff16bitLock);
    if (nullptr != m_markerTiff16bit) {
        delete [] m_markerTiff16bit;
        m_markerTiff16bit = nullptr;
    }

    return (m_camera->getMarkerImage(&m_markerTiff16bit) != ERROR_NONE);
}

void WzCaptureService::clearMarkerImage()
{
    //m_autoGotMarker = false;
    m_markerImageSize.setHeight(0);
    m_markerImageSize.setWidth(0);
    QMutexLocker lock(&m_markerTiff16bitLock);
    if (nullptr != m_markerTiff16bit) {
        delete [] m_markerTiff16bit;
        m_markerTiff16bit = nullptr;
    }    
};

QString WzCaptureService::saveMarkerImage(const QDateTime &dateTime) {
    QMutexLocker lock(&m_markerTiff16bitLock);

    if (nullptr == m_markerTiff16bit)
        return "";

    QDateTime now = dateTime;
    QString dateStr = now.toString("yyyy-MM-dd");
    QString dateTimeStr = now.toString("yyyy-MM-dd_hh-mm-ss");

    // make the sub path, for example: 2020-01-13/.sh_marker
    QDir markerPath(m_imagePath);
    markerPath.mkdir(dateStr);
    markerPath.cd(dateStr);
    markerPath.mkdir(kMarkerFolder);
    markerPath.cd(kMarkerFolder);

    // set the marker's folder to hide if the OS is windows
    #ifdef WINNT
    std::wstring path1 = markerPath.absolutePath().toStdWString();
    SetFileAttributes(path1.data(), FILE_ATTRIBUTE_HIDDEN);
    #endif

    // save the marker image with 16bit tiff

    QString markerFileName = "marker_" + dateTimeStr + ".tif";
    QFileInfo markerFileInfo = QFileInfo(markerPath, markerFileName);

    WzImageBuffer buf;
    buf.width = m_markerImageSize.width();
    buf.height = m_markerImageSize.height();
    buf.bitDepth = 16;
    buf.buf = reinterpret_cast<uint8_t*>(m_markerTiff16bit);
    buf.update();

    if (m_markerTiff16bit[100 * m_markerImageSize.width() + 100] == 0) {
        qWarning() << "saveMarkerImage, pixel[100,100] == 0";
    }

    uint16_t* newBuffer = nullptr;
    QVariant chemiImageWidth, chemiImageHeight;
    m_camera->getParam("imageWidth", chemiImageWidth);
    m_camera->getParam("imageHeight", chemiImageHeight);
    if (chemiImageWidth != buf.width || chemiImageHeight != buf.height) {
        qreal zoom = 1;
        int newWidth, newHeight;
        newWidth = buf.width;
        newHeight = buf.height;
        zoom = static_cast<qreal>(chemiImageWidth.toInt()) / static_cast<qreal>(buf.width);
        if (zoom != 1) {
            newBuffer = WzAbstractCamera::changeImageSize(buf.bit16Array, newWidth, newHeight, zoom);
            buf.width = newWidth;
            buf.height = newHeight;
            buf.buf = reinterpret_cast<uint8_t*>(newBuffer);
            buf.bytesCountOfBuf = buf.width * buf.height * sizeof(uint16_t);
            buf.update();

            if (newBuffer[100 * newWidth + 100] == 0) {
                qDebug() << "saveMarkerImage, newBuffer, pixel[100,100] == 0";
            }
        }
    }

    WzImageService::saveImageAsTiff(buf, markerFileInfo.absoluteFilePath());

    if (nullptr != newBuffer)
        delete [] newBuffer;
    return markerFileInfo.absoluteFilePath();
}

void WzCaptureService::startAutoFocus()
{
    m_autoFocus.start();
}

void WzCaptureService::stopAutoFocus()
{
    m_autoFocus.stop();
}

QString WzCaptureService::getPreviewImageDiff(const int diffType)
{
    QString result;

    if (diffType == 0) {
        if (m_camera) {
            m_camera->getMarkerImage(&m_markerTiff16bit);
            QRect r;
            r.setWidth(m_previewImageSize->width());
            r.setHeight(m_previewImageSize->height());
            auto diff = m_autoFocus.getDiffCount(m_markerTiff16bit, *m_previewImageSize, r);
            result = QString::number(diff);
        }
    }
    else {
        if (m_previewImageData) {
            QRect r;
            r.setWidth(m_previewImageSize->width());
            r.setHeight(m_previewImageSize->height());
            auto diff = m_autoFocus.getDiffCount(m_previewImageData, *m_previewImageSize, r);
            result = QString::number(diff);
        }
    }

    return result;
}

void WzCaptureService::readCameraSNList() {
    QString fileName = QApplication::applicationDirPath() + "/" + kCameraSNFile;

#ifndef HARDLOCK
    return;
#else
    aes256_key key;

    char vid[9];
    vid[8] = '\0';
    for(int i = 0; i < 9; i++) {
        switch(i) {
        case 0: vid[0] = 0x34; break;
        case 1: vid[1] = 0x32; break;
        case 2: vid[2] = 0x38; break;
        case 3: vid[3] = 0x30; break;
        case 4: vid[4] = 0x35; break;
        case 5: vid[5] = 0x39; break;
        case 6: vid[6] = 0x32; break;
        case 7: vid[7] = 0x38; break;
        }
    }

    RY_HANDLE handle;
    int count = 0;
    if (RY3_SUCCESS != RY3_Find(vid, &count)) return;
    if (count == 0) return;
    unsigned long ret = WzRenderThread::getHandle(count, &handle);
    if (RY3_SUCCESS != ret)
        return;

    unsigned char inBuf[] =
        { 0xb6, 0x8f, 0x76, 0x15, 0x28, 0xc6, 0xc6, 0x0e, 0xb5, 0x71, 0xd5, 0x63, 0x86, 0x41, 0x6c, 0x5d
        , 0xb8, 0xa9, 0x55, 0xab, 0xe5, 0xf7, 0xe8, 0x7f, 0xfa, 0xce, 0xff, 0xaa, 0xb1, 0x84, 0xeb, 0xa1
        , 0x92, 0xa1, 0xa9, 0x27, 0x9d, 0x1d, 0x05, 0x42, 0x9f, 0x16, 0x02, 0xa9, 0xc6, 0x33, 0x03, 0x9c
        , 0x9d, 0x09, 0xa6, 0xcc, 0xd9, 0x47, 0x16, 0x01, 0xc9, 0xcd, 0xbf, 0xe9, 0x0f, 0x5a, 0x5e, 0x2f
        , 0x49, 0x08, 0x50, 0xf1, 0x4d, 0x66, 0x9d, 0xd6, 0xfb, 0xa1, 0x6e, 0xc4, 0xa2, 0xd9, 0xe7, 0x77
        , 0x83, 0x0e, 0x40, 0x80, 0xca, 0x21, 0xaa, 0xc0, 0x17, 0xae, 0xe6, 0xea, 0xef, 0xbc, 0x03, 0x24
        , 0x14, 0xc2, 0x60, 0x40, 0x54, 0x1b, 0x19, 0x20, 0xd2, 0x60, 0xab, 0x9a, 0xd7, 0xba, 0x4f, 0x87
        , 0x60, 0x76, 0x25, 0x69, 0x6a, 0xd7, 0x4e, 0x6f, 0xf1, 0x15, 0xce, 0x89, 0x14, 0x8e, 0x17, 0xde};
    const int OUT_BUF_SIZE = 255;
    int outBufSize = OUT_BUF_SIZE;
    unsigned char outBuf[OUT_BUF_SIZE];
    int retcode = RY3_ExecuteFile(handle, 0x0004, inBuf, 128, outBuf, &outBufSize);
    if (RY3_SUCCESS != retcode)
        return;

    if (outBufSize != 255) return;
    memcpy_s(key, AES256_KEY_LEN, &outBuf[55], AES256_KEY_LEN);//获取相机传来的sn号给key

    char* buf = nullptr;
    int bufSize = 0;

    if (!WzUtils::aesDecryptFileToBuf(key, fileName, &buf, &bufSize))//在aes加密下拿出所有sn号
        return;

    QTextStream textStream(buf);

    m_cameraSNList.clear();
    while(!textStream.atEnd()) {
        m_cameraSNList << textStream.readLine();
    }

    delete []buf;
#endif
}

QString WzCaptureService::imagePath() {
    return m_imagePath;
};

void WzCaptureService::setImagePath(const QString imagePath) {
    m_imagePath = imagePath;
    emit imagePathChanged();
};

WzCameraState::CameraState WzCaptureService::cameraState() {
    return m_cameraState;
}

bool WzCaptureService::getAutoExposure() {
    return m_isAutoExposure;
};

void WzCaptureService::setAutoExposure(bool autoExposure) {
    m_isAutoExposure = autoExposure;
    emit autoExposureChanged();
};

int WzCaptureService::getCameraTemperature() {
    if (m_cameraConnected) {
        return static_cast<int>(m_camera->getTemperature());
    } else {
        return 0;
    }
}

void WzCaptureService::setCameraParam(const QString &paramName, const QVariant paramVal)
{
    m_camera->setParam(paramName, paramVal);
}

QVariant WzCaptureService::getCameraParam(const QString &paramName)
{
    if (m_camera) {
        QVariant param;
        m_camera->getParam(paramName, param);
        return param;
    }
    return QVariant();
}

bool WzCaptureService::hasMarkerImage(int threshold)
{
    QMutexLocker lock(&m_markerTiff16bitLock);
    if (nullptr == m_markerTiff16bit) {
        qWarning() << "hasMarkerImage, nullptr";
        return false;
    }
    if (m_markerImageSize.width() < 1) {
        qWarning() << "hasMarkerImage, width = 0";
        return false;
    }
    if (m_markerImageSize.height() < 1) {
        qWarning() << "hasMarkerImage, height = 0";
        return false;
    }

    if (threshold > 0) {
        // 按照一定间距计算N行像素的平均灰阶, 低于特定值则判断白光图亮度不够, 并且视为没有白光图
        int rowStep = static_cast<double>(m_markerImageSize.height()) / 5.0;
        int64_t grays = 0;
        int pixelCount = 0;
        for (int row = rowStep; row < m_markerImageSize.height(); row += rowStep) {
            for (int col = 0; col < m_markerImageSize.width(); col++) {
                pixelCount++;
                grays += m_markerTiff16bit[row * m_markerImageSize.width() + col];
            }
        }
        if (static_cast<double>(grays) / static_cast<double>(pixelCount) < threshold) {
            qWarning() << "hasMarkerImage, grays = " << grays << ", pixelCount = " << pixelCount;
            return false;
        }
    }

    return true;
}

int WzCaptureService::getAutoExposureMs() {
    if (m_camera != nullptr)
        return m_camera->getExposureMs();
    else
        return 0;
}

bool WzCaptureService::getStorageWhite()
{
    return m_StorageWhite;
}

void WzCaptureService::setStorageWhite(bool StorageWhite)
{
    m_StorageWhite=StorageWhite;
    if (m_camera != nullptr) {
        m_camera->setParam("StorageWhite", StorageWhite);
    }
    emit StorageWhiteChanged();
    emit SendStorageWhite(StorageWhite);
}
