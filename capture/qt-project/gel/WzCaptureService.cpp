#include "WzCaptureService.h"

WzCaptureService::WzCaptureService(QObject *parent):
    QObject(parent),
    m_previewImageData(nullptr),
    m_previewImageSize(new QSize(100, 100)),
    m_captureTimer(new QTimer())
{
    qDebug() << "WzCaptureService";

#ifdef WIN32
#ifdef DEMO
    m_camera = new WzTestCamera(parent);
#else
#ifdef GEL_CAPTURE
#ifdef TOUPCAM
    uint32_t toupcamCount = ShstToupCamera::count();
    qInfo() << "Toupcam count:" << toupcamCount;
    if (toupcamCount > 0) {
        m_camera = dynamic_cast<WzAbstractCamera*>(new ShstToupCamera(parent));
        setActiveCameraManufacturer("Toup");
    }
#endif
    if (nullptr == m_camera)
        m_camera = dynamic_cast<WzAbstractCamera*>(new WzKsjCamera(parent));
#else
    m_camera(new WzPvCamera(parent)),
#endif
#endif
#endif
#ifdef MAC
    m_camera  = new WzTestCamera(parent);
#endif

    readCameraSNList();

    QObject::connect(m_camera, SIGNAL(previewImageUpdated()), this, SLOT(previewImageUpdated()));
    QObject::connect(m_camera, SIGNAL(cameraState(WzCameraState::CameraState)), this, SLOT(cameraStateUpdated(WzCameraState::CameraState)));
    QObject::connect(m_captureTimer, SIGNAL(timeout()), this, SLOT(captureTimer()));

    m_captureTimer->setSingleShot(false);
    m_captureTimer->setInterval(900);

    m_camera->setCameraCallback(this);


    connect(&m_autoFocus, &WzAutoFocus::focusFar, this, &WzCaptureService::autoFocusFar);
    connect(&m_autoFocus, &WzAutoFocus::focusNear, this, &WzCaptureService::autoFocusNear);
    connect(&m_autoFocus, &WzAutoFocus::focusStop, this, &WzCaptureService::autoFocusStop);
    connect(&m_autoFocus, &WzAutoFocus::getImage, this, &WzCaptureService::autoFocusGetImage);
    connect(&m_autoFocus, &WzAutoFocus::log, this, &WzCaptureService::autoFocusLog);
    connect(&m_autoFocus, &WzAutoFocus::finished, this, &WzCaptureService::autoFocusFinished);

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
    if (nullptr != m_markerImageData) {
        delete[] m_markerImageData;
        m_markerImageData = nullptr;
    }
}

bool WzCaptureService::enableCameraPreview(bool enabled, int binning) {
    if (!m_camera)
        return false;
    m_camera->setParam("Binning", binning);
    if (enabled) {
        if (m_activeCameraManufacturer == "Toup")
            g_liveImageView->setFlipHoriz(true);
        else
            g_liveImageView->setFlipHoriz(false);
    }
    return ERROR_NONE == m_camera->setPreviewEnabled(enabled);
}

// TODO 将此处代码转移到 capture(QVariantMap params)
bool WzCaptureService::capture(int exposureMilliseconds, int binning) {
    qDebug() << "WzCaptureService::capture, ExposureMs:" << exposureMilliseconds;
    m_captureId = 0;
    m_captureCount = 1;
    m_captureIndex = 0;
    emit this->captureState(Init);
    m_camera->setParam("ImagePath", m_imagePath);
    m_camera->setParam("Binning", binning);
    m_camera->setParam("AE", m_isAutoExposure);
    bool ret = m_camera->capture(qMax(1, exposureMilliseconds));
    m_captureTimer->start();
    return ret;
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

    QString lt = "";
    if (params.contains("openedLightType"))
        lt = params["openedLightType"].toString();
    if (params["isLightOpened"].toBool() &&
            (lt == "uv_penetrate" || lt == "uv_penetrate_force" ||
             lt == "white_down" || lt == "red" || lt == "green")) {
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

    QList<int> exposureMsList;
    if (params["multiType"] == "gray") {
        m_captureCount = params["grayAccumulate_frameCount"].toInt();
        m_camera->setParam("grayAccumulate", true);
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
             lt == "white_down" || lt == "red" || lt == "green")) {
        m_camera->setParam("isThumbNegative", false);
    } else {
        m_camera->setParam("isThumbNegative", true);
    }

    qDebug() << "exposureMsList:" << exposureMsList;
    int* exposureMsArray = new int[exposureMsList.count()];
    for (int i = 0; i < exposureMsList.count(); i++) {
        exposureMsArray[i] = qMin(exposureMsList[i], 10 * 1000);
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
    int imageBytes = m_camera->getImageBytes();

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
            m_previewImageData = new uchar[imageBytes];
        }

        m_camera->getImage(m_previewImageData);
    }

    if (imageSize->width() != this->m_markerImageSize.width() ||
            imageSize->height() != this->m_markerImageSize.height()) {
        if (nullptr != m_markerImageData) {
            delete[] m_markerImageData;
            m_markerImageData = nullptr;
        }
    }
    m_markerImageSize.setWidth(imageSize->width());
    m_markerImageSize.setHeight(imageSize->height());
    if (nullptr == m_markerImageData) {
        m_markerImageData = new uchar[imageBytes];
    }
    memcpy(m_markerImageData, m_previewImageData, static_cast<size_t>(imageBytes));

    if (g_liveImageView) {
        if (m_camera->isRgbImage())
            g_liveImageView->updateImage(m_previewImageData, *imageSize, imageBytes, QImage::Format_RGB888);
        else
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
        QVariant captureDateTime;
        m_camera->getParam("captureDateTime", captureDateTime);
        QVariant sampleName;
        m_camera->getParam("SampleName", sampleName);
        QString markerImageFilename = saveMarkerImage(captureDateTime.toDateTime());
        WzDatabaseService db;
        QVariantMap imageInfo;
        imageInfo["imageFile"] = m_latestImageFile;
        imageInfo["imageThumbFile"] = m_latestThumbFile;
        imageInfo["imageWhiteFile"] = markerImageFilename;
        imageInfo["imageSource"] = WzEnum::ImageSource::Capture;
        imageInfo["exposureMs"] = m_camera->getExposureMs();
        imageInfo["groupId"] = m_captureId;
        imageInfo["captureDate"] = captureDateTime.toDateTime();
        imageInfo["sampleName"] = sampleName.toString();

        QVariant isThumbNegative = true;
        m_camera->getParam("isThumbNegative", isThumbNegative);

        if (m_captureParams.contains("openedLightType") && m_captureParams.contains("isLightOpened")) {
            if (m_captureParams["isLightOpened"] == true) {
                imageInfo["openedLight"] = m_captureParams["openedLightType"];
                if (imageInfo["openedLight"] == "uv_penetrate" || imageInfo["openedLight"] == "uv_penetrate_force" ||
                        imageInfo["openedLight"] == "red" || imageInfo["openedLight"] == "green" || imageInfo["openedLight"] == "blue")
                    imageInfo["imageInvert"] = 0x02;
                else if (imageInfo["openedLight"] == "white_down")
                    imageInfo["imageInvert"] = 0x04;
                else if (imageInfo["openedLight"] == "white_up")
                    imageInfo["imageInvert"] = 0x02;
                else
                    imageInfo["imageInvert"] = 0x01;
            } else {
                imageInfo["openedLight"] = "";
                imageInfo["imageInvert"] = 0x01;
            }
        } else {
            imageInfo["openedLight"] = "";
            imageInfo["imageInvert"] = 0x01;
        }

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
    if (m_cameraSNList.contains(sn))
        return true;
    else {
        qInfo() << "未许可的序列号: " << sn;
        return false;
    }
}

void WzCaptureService::abortCapture() {
    m_camera->abortCapture();
}

int WzCaptureService::getCameraCount() {
    if (m_camera)
        return 1; // 暂时硬编码
    return 0;
}

void WzCaptureService::setPreviewExposureMs(int exposureMs) {
    if (!m_camera)
        return;
    m_camera->setParam("ExposureMs", exposureMs);
};

QString WzCaptureService::saveMarkerImage(const QDateTime &dateTime) {
    if (nullptr == m_markerImageData)
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

    QString markerFileName = "marker_" + dateTimeStr + ".jpg";
    QFileInfo markerFileInfo(markerPath, markerFileName);

    // save the marker's image to file
    QImage markerJpeg(m_markerImageSize.width(), m_markerImageSize.height(), QImage::Format_Grayscale8);
    memcpy(markerJpeg.bits(), m_markerImageData,
           static_cast<size_t>(m_markerImageSize.width() * m_markerImageSize.height()));
    markerJpeg.save(markerFileInfo.absoluteFilePath(), "jpg", 75);

    return markerFileInfo.absoluteFilePath();
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
    if (RY3_SUCCESS != WzRenderThread::getHandle(count, &handle)) return;

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
    memcpy_s(key, AES256_KEY_LEN, &outBuf[55], AES256_KEY_LEN);

    char* buf = nullptr;
    int bufSize = 0;

    if (!WzUtils::aesDecryptFileToBuf(key, fileName, &buf, &bufSize))
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

void WzCaptureService::startAutoFocus()
{
    m_autoFocus.start();
}

void WzCaptureService::stopAutoFocus()
{
    m_autoFocus.stop();
}

int WzCaptureService::getAutoExposureMs() {
    if (m_camera != nullptr)
        return m_camera->getExposureMs();
    else
        return 0;
}

QString WzCaptureService::activeCameraManufacturer() const
{
    return m_activeCameraManufacturer;
}

void WzCaptureService::setActiveCameraManufacturer(const QString &newValue)
{
    if (m_activeCameraManufacturer == newValue)
        return;
    m_activeCameraManufacturer = newValue;
    emit activeCameraManufacturerChanged();
}
