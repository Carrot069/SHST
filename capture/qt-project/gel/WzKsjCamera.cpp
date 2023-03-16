#include "WzKsjCamera.h"

// TODO 监测相机断开事件
// TODO 观察是否需要进行镜头校正

void WINAPI KSJPREVIEWCALLBACK(unsigned char *pImageData, int nWidth, int nHeight, int nBitCount, void *lpContext)
{
    WzKsjCamera* ctx = static_cast<WzKsjCamera*>(lpContext);
    ctx->newFrame(pImageData, nWidth, nHeight, nBitCount);
}

WzKsjCamera::WzKsjCamera(QObject *parent) :
    WzAbstractCamera(parent),
    m_isConnected(false),
    m_run(false),
    m_cameraState(WzCameraState::Connecting),
    m_isPreviewEnabled(false),
    m_isPreviewUpdated(false),
    m_isCapture(false),
    m_imageSize(new QSize(672, 550))
{
    qDebug() << "WzKsjCamera";

    m_cameraParameters["ExposureMs"] = 10;
    m_cameraParameters["Binning"   ] = 1;

    m_timer = new QTimer(this);
    m_getTemperatureTimer = new QTimer(this);
    m_thread = new WzCameraThread(this);

    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(handleTimerFired()));
    m_timer->setSingleShot(false);
    m_timer->setInterval(10);
    m_timer->start();

    // 白光预览的灰阶显示范围 0 - 20000, 这样可以降低曝光时间, 加快单帧曝光时间
    for(uint i = 0; i < 65536; i++)
        m_gray16to8Table[i] = static_cast<uchar>(qMin<uint>(i / 80, 255));
}

WzKsjCamera::~WzKsjCamera()
{
    m_run = false;
    m_thread->wait(5000);
    delete m_imageSize;
    delete m_thread;
    delete m_timer;

    if (m_previewImage8bit != nullptr) {
        delete[] m_previewImage8bit;
        m_previewImage8bit = nullptr;
    }
    if (nullptr != m_previewImage16bit) {
        delete [] m_previewImage16bit;
        m_previewImage16bit = nullptr;
    }

    if (nullptr != m_previewBuffer) {
        delete [] m_previewBuffer;
        m_previewBuffer = nullptr;
    }

    if (nullptr != m_exposureMilliseconds) {
        delete[] m_exposureMilliseconds;
        m_exposureMilliseconds = nullptr;
    }

    if (nullptr != m_grayAccumulateBuffer) {
        delete[] m_grayAccumulateBuffer;
        m_grayAccumulateBuffer = nullptr;
    }
}

void WzKsjCamera::connect()
{
    m_cameraState = WzCameraState::Connecting;
    emit cameraState(m_cameraState);
    m_thread->start();
}

void WzKsjCamera::disconnect()
{
    m_run = false;
    m_thread->wait(5000);
    return;
}

void WzKsjCamera::run()
{
    bool isStartedCapture = false;
    bool isStartedExposure = false;
    bool isStartedPreview = false;
    int currentPreviewExposureMs = 0;
    QThread::msleep(1000);
    m_isConnected = false;
    m_run = true;
    m_cameraState = WzCameraState::Connecting;
    emit cameraState(m_cameraState);

    if (nullptr == m_cameraCallback) {
        qWarning("nullptr == m_cameraCallback");
    } else {
        // TODO
        if (initKSJ()) {
            if (openCamera()) {
                m_isConnected = true;
                m_cameraState = WzCameraState::Connected;
                emit cameraState(m_cameraState);
            }
        } else {
            m_cameraState = WzCameraState::CameraNotFound;
            emit cameraState(m_cameraState);
            uninitKSJCAM();
        }
    }

    while (m_run) {
        if (!m_isCameraOpen) {
            QThread::msleep(10);
            continue;
        }
        if (m_isPreviewEnabled) {
            if (!isStartedPreview) {
                if (setupPreview() && startPreview()) {
                    QVariant exposureMs = 1;
                    getParam("ExposureMs", exposureMs);
                    currentPreviewExposureMs = exposureMs.toInt();
                    isStartedPreview = true;
                    m_cameraState = WzCameraState::PreviewStarted;
                    emit cameraState(m_cameraState);
                } else {
                    m_isPreviewEnabled = false;
                    m_cameraState = WzCameraState::PreviewStopped;
                    emit cameraState(m_cameraState);
                    continue;
                }
            } else {
                uint8_t *frameBytes = nullptr;
                if (getLatestFrame(&frameBytes)) {
                    QMutexLocker lock(&m_mutexImageData);
                    m_pImageData = m_previewBuffer;
                    m_isPreviewUpdated = true;
                }

                QVariant exposureMs = 1;
                getParam("ExposureMs", exposureMs);
                if (exposureMs.toInt() != currentPreviewExposureMs) {
                    currentPreviewExposureMs = exposureMs.toInt();
                    stopPreview();
                    startPreview();
                }
            }
        } else {
            if (isStartedPreview) {
                isStartedPreview = false;
                if (stopPreview()) {
                    m_cameraState = WzCameraState::PreviewStopped;
                    emit cameraState(m_cameraState);
                } else {
                    // TODO 处理停止预览失败的情况
                }
            }
        }
        if (m_isCapture) {
            // 如果正在预览且预览时间与拍摄时间相同则保存最后一帧预览画面当作拍摄图片
            if (m_cameraState == WzCameraState::PreviewStarted) {
                m_isPreviewEnabled = false;
                m_cameraState = WzCameraState::PreviewStopping;
                emit cameraState(m_cameraState);
                if (stopPreview()) {
                    m_cameraState = WzCameraState::PreviewStopped;
                    emit cameraState(m_cameraState);
                } else {
                    // TODO 处理停止失败
                }
                // 将最后一帧预览画面复制到单帧拍摄
                copyPreviewToSingleFrame();
            }
            // 拍摄前的初始化
            if (!isStartedCapture) {
                m_cameraState = WzCameraState::CaptureInit;
                emit cameraState(m_cameraState);

                singleCaptureSetup();
                m_capturedCount = 0;
                m_currentFrame = 0;
                m_leftExposureMilliseconds = m_exposureMilliseconds[m_currentFrame];

                isStartedCapture = true;
            }
            if (m_isAbortCapture) {
                qInfo("Check abort single capture in thread");
                m_isAbortCapture = false;
                m_isCapture = false;
                isStartedCapture = false;
                if (isStartedExposure) {
                    // TODO 考虑 如果失败了 该如何处理，或者不处理
                    singleCaptureAbort();
                    singleCaptureFinished();
                }
                isStartedExposure = false;
                m_cameraState = WzCameraState::CaptureAborted;
                emit cameraState(m_cameraState);
                continue;
            }
            if (isStartedCapture && !isStartedExposure) {
                m_leftExposureMilliseconds = -1;
                m_cameraState = WzCameraState::Exposure;
                emit cameraState(m_cameraState);
                // TODO 考虑拍摄失败后的处理方案
                if (singleCaptureStart(static_cast<uint32_t>(m_exposureMilliseconds[m_currentFrame]))) {
                    //isStartedExposure = true;
                } else {
                    m_cameraState = WzCameraState::Error;
                    emit cameraState(m_cameraState);
                    m_isCapture = false;
                    isStartedCapture = false;
                    isStartedExposure = false;
                    continue;
                }
            }

            // 全部拍完了
            if (m_currentFrame+1 == m_frameCount) {
                m_isCapture = false;
                m_isCaptureFinished = true;
                isStartedCapture = false;
            }

            m_cameraState = WzCameraState::Image;
            emit cameraState(m_cameraState);

            if (m_isAbortCapture) {
                qInfo("Check abort single capture in thread");
                m_isAbortCapture = false;
                m_isCapture = false;
                isStartedCapture = false;
                isStartedExposure = false;
                // TODO 考虑 如果失败了 该如何处理，或者不处理
                singleCaptureAbort();
                singleCaptureFinished();
                m_cameraState = WzCameraState::CaptureAborted;
                emit cameraState(m_cameraState);
                continue;
            }

            isStartedExposure = false;
            singleCaptureFinished();

            {
                QMutexLocker lock(&m_mutexImageData);
                m_pImageData = m_singleFrame;
                processImage();
            }

            m_capturedCount++;
            m_cameraState = WzCameraState::CaptureFinished;
            emit cameraState(m_cameraState);
            QThread::msleep(50); // 必须停一下, 防止上层逻辑检测不到这个状态就变掉了

            if (m_currentFrame+1 < m_frameCount) {
                m_currentFrame++;
                m_leftExposureMilliseconds = m_exposureMilliseconds[m_currentFrame];
            }

        }

        QThread::msleep(10);
    }

    m_cameraState = WzCameraState::Disconnecting;
    emit cameraState(m_cameraState);

    QThread::msleep(1000);
    m_cameraState = WzCameraState::Disconnected;
    emit cameraState(m_cameraState);

    closeCamera();
    uninitKSJCAM();
}

int WzKsjCamera::setPreviewEnabled(bool enabled)
{
    if (!m_isConnected)
        return ERROR_CAM_NOT_CONNECTED;
    qDebug() << "setPreviewEnabled, enabled =" << enabled;
    if (enabled) {
        m_cameraState = WzCameraState::PreviewStarting;
        emit cameraState(m_cameraState);
        QVariant binning = 1;
//        getParam("Binning", binning);
//        if (binning > 4) binning = 4;
//        if (binning < 1) binning = 1;
        m_imageSize->setWidth(m_SensorResX / binning.toInt());
        m_imageSize->setHeight(m_SensorResY / binning.toInt());
        if (nullptr != m_previewImage8bit) {
            delete[] m_previewImage8bit;
            m_previewImage8bit = nullptr;
        }
        qDebug() << "\tstep1, w:" << m_imageSize->width() <<
            ", h:" << m_imageSize->height();
        m_previewImage8bit = new uchar[m_imageSize->width() * m_imageSize->height()];
        m_imageBytes = static_cast<uint>(m_imageSize->width() * m_imageSize->height());
        qDebug() << "\tstep2";
    } else {
        m_cameraState = WzCameraState::PreviewStopping;
        emit cameraState(m_cameraState);
    }
    m_isPreviewEnabled = enabled;
    qDebug() << "\tend";
    return ERROR_NONE;
}

int WzKsjCamera::getImage(void* imageData)
{
    QMutexLocker lock(&m_mutexImageData);
    memcpy(imageData, m_pImageData, static_cast<size_t>(m_imageBytes));
    return ERROR_NONE;
}

int WzKsjCamera::capture(const int exposureMilliseconds)
{
    int ms[] = {exposureMilliseconds};
    return capture(ms, 1);
}

int WzKsjCamera::capture(const int* exposureMilliseconds, const int count)
{
    if (count == 0) return ERROR_PARAM;
    m_frameCount = count;
    if (m_exposureMilliseconds)
        delete[] m_exposureMilliseconds;
    m_exposureMilliseconds = new int[count];
    memcpy(m_exposureMilliseconds, exposureMilliseconds, static_cast<uint>(count) * sizeof(int));
    m_isAbortCapture = false;
    m_isCapture = true;
    return ERROR_NONE;
}

void WzKsjCamera::abortCapture() {
    qDebug() << "WzKsjCamera::abortCapture(), m_isAbortCapture:" << m_isAbortCapture;
    if (m_isAbortCapture)
        return;
    m_isAbortCapture = true;
    m_cameraState = WzCameraState::CaptureAborting;
    emit cameraState(m_cameraState);
    qDebug() << "WzKsjCamera::abortCapture()";
}

void WzKsjCamera::handleTimerFired()
{
    if (m_isPreviewEnabled && m_isPreviewUpdated) {
        emit previewImageUpdated();
        m_isPreviewUpdated = false;
    } else if (m_isCaptureFinished) {
        emit captureFinished();
        m_isCaptureFinished = false;
    }
}

QSize* WzKsjCamera::getImageSize() {
    return m_imageSize;
}

int WzKsjCamera::getExposureMs() {
    return m_exposureMilliseconds[m_currentFrame];
}

int WzKsjCamera::getLeftExposureMs() {
    return m_leftExposureMilliseconds;
}

int WzKsjCamera::getCurrentFrame() {
    return m_currentFrame;
}

int WzKsjCamera::getCapturedCount() {
    return m_capturedCount;
}

int WzKsjCamera::getImageBytes() {
    return static_cast<int>(m_imageBytes);
}

void WzKsjCamera::processImage() {
    WzImageBuffer imageBuffer;
    imageBuffer.buf = m_singleFrame;
    imageBuffer.width = static_cast<uint32_t>(m_imageSize->width());
    imageBuffer.height = static_cast<uint32_t>(m_imageSize->height());
    if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500C_MRYY) {
        imageBuffer.bitDepth = 8;
        imageBuffer.samplesPerPixel = 3;
    } else {
        imageBuffer.bitDepth = 16;
        imageBuffer.samplesPerPixel = 1;
    }
    imageBuffer.bytesCountOfBuf = static_cast<uint32_t>(m_imageBytes);
    imageBuffer.exposureMs = static_cast<uint32_t>(getExposureMs());
    imageBuffer.captureDateTime = QDateTime::currentDateTime();

    setParam("captureDateTime", imageBuffer.captureDateTime);

    // 灰阶位数拉伸
    imageBuffer.update();
    if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UD205M_SGYY) {
        for (uint32_t n = 0; n < imageBuffer.width * imageBuffer.height; n++) {
            imageBuffer.bit16Array[n] = (imageBuffer.bit16Array[n] + 1) * 16 - 1;
        }
    } else if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500M_MRNN ||
               m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500M_MRYY) {
        uint16_t* buffer16Bit = new uint16_t[imageBuffer.width * imageBuffer.height];
        for (uint32_t n = 0; n < imageBuffer.height * imageBuffer.width; n++) {
            buffer16Bit[n] = (m_singleFrame[n] + 1) * 256 - 1;
        }
        delete [] m_singleFrame;
        m_singleFrame = reinterpret_cast<uint8_t*>(buffer16Bit);
        imageBuffer.buf = reinterpret_cast<uint8_t*>(buffer16Bit);
        imageBuffer.update();
    }


    // 处理灰度累积
    QVariant isGrayAccumulate = false;
    if (getParam("grayAccumulate", isGrayAccumulate) && isGrayAccumulate == true) {
        if (nullptr == m_grayAccumulateBuffer) {
            qFatal("m_grayAccumulateBuffer == nullptr");
        } else {
            for (uint i = 0; i < imageBuffer.width * imageBuffer.height; i++) {
                int newGray = imageBuffer.bit16Array[i] + m_grayAccumulateBuffer[i];
                if (newGray > 65535)
                    newGray = 65535;
                imageBuffer.bit16Array[i] = newGray;
                m_grayAccumulateBuffer[i] = newGray;
            }
        }
    }

    // TODO 降噪、畸变校正、处理背景


    QString path = mkdirImagePath();
    m_latestImageFile = getTiffFileName(path, imageBuffer.captureDateTime);

    // 在当前路径下的 .sh_thumb 文件夹下生成同名缩略图
    imageBuffer.update();
    QVariant isThumbNegative = true;
    getParam("isThumbNegative", isThumbNegative);
    if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500C_MRYY)
        m_latestThumbFile = WzImageService::createThumbRGB(imageBuffer, m_latestImageFile);
    else
        m_latestThumbFile = WzImageService::createThumb(imageBuffer, m_latestImageFile, isThumbNegative.toBool());

    uint16_t* oldBuffer = imageBuffer.bit16Array;
    int newWidth, newHeight;
    newWidth = imageBuffer.width;
    newHeight = imageBuffer.height;
    qreal zoom = 0;
    if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UD205M_SGYY)
        zoom = 4;
    else if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500M_MRNN ||
             m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500M_MRYY)
#ifdef UC500_2000
        zoom = 2;
#else
        zoom = 1;
#endif
    else
        zoom = 1;
    if (zoom != 1) {
        uint16_t* newBuffer = changeImageSize(imageBuffer.bit16Array, newWidth, newHeight, zoom);
        delete []oldBuffer;
        imageBuffer.width = newWidth;
        imageBuffer.height = newHeight;
        imageBuffer.buf = reinterpret_cast<uint8_t*>(newBuffer);
        m_singleFrame = reinterpret_cast<uint8_t*>(newBuffer);
        imageBuffer.bytesCountOfBuf = imageBuffer.width * imageBuffer.height * sizeof(uint16_t);
        imageBuffer.update();
    }

    WzImageService::saveImageAsTiff(imageBuffer, m_latestImageFile);
}


QString WzKsjCamera::getLatestImageFile() {
    return m_latestImageFile;
};

QString WzKsjCamera::getLatestThumbFile() {
    return m_latestThumbFile;
}

double WzKsjCamera::getTemperature() {
    return 0;
}

bool WzKsjCamera::isRgbImage() const
{
    if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500C_MRYY) {
        return true;
    } else {
        return false;
    }
}

bool WzKsjCamera::initKSJ() {
    int retCode = RET_SUCCESS;

    retCode = KSJ_Init();
    if (checkRet(retCode, "KSJ_Init"))
        return false;

    m_isCamInitialized = true;
    qInfo("KSJ initialized");

    m_numberOfCameras = KSJ_DeviceGetCount();

    if (m_numberOfCameras == 0) {
        qInfo("No cameras found in the system");
        return false;
    }
    qInfo("Number of cameras found: %d", m_numberOfCameras);

    m_numberOfCameras = KSJ_DeviceGetCount();
    qDebug("%d KSJ Device Found:", m_numberOfCameras);

    for (int i = 0; i < m_numberOfCameras; i++) {
        m_deviceInfo[i].nIndex = i;
        retCode = KSJ_DeviceGetInformationEx(
                    i,
                    &(m_deviceInfo[i].DeviceType),
                    &(m_deviceInfo[i].nSerials),
                    &(m_deviceInfo[i].wFirmwareVersion),
                    &(m_deviceInfo[i].wFpgaVersion));
        checkRet(retCode, "KSJ_DeviceGetInformationEx");
    }

    return true;
}

bool WzKsjCamera::uninitKSJCAM() {
    // Uninitialize KSJ library
    if (m_isCamInitialized) {
        int retCode = KSJ_UnInit();
        if (checkRet(retCode, "KSJ_UnInit")) {
            return false;
        } else {
            m_isCamInitialized = false;
        }
        qInfo("KSJ uninitializated");
    }

    return true;
}

bool WzKsjCamera::openCamera() {
    // TODO 大于0时用Mutex的形式等待UI弹出窗口中选择相机后再连接
    int retCode = RET_SUCCESS;

    KSJ_GetParamRange(m_deviceCurSel, KSJ_EXPOSURE, &m_minExposure, &m_maxExposure);
    qDebug() << "KSJ Max Exposure: " << m_maxExposure;
    if (m_maxExposure == 0)
        return false;

    retCode = KSJ_PreviewSetCallback(m_deviceCurSel, KSJPREVIEWCALLBACK, this);
    if (checkRet(retCode, "KSJ_PreviewSetCallback"))
        return false;

    int isSupport = 0;
    retCode = KSJ_QueryFunction(m_deviceCurSel, KSJ_SUPPORT_16BITS, &isSupport);
    if (checkRet(retCode, "KSJ_QueryFunction(KSJ_SUPPORT_16BITS)"))
        return false;
    if (isSupport) {
        retCode = KSJ_SetData16Bits(m_deviceCurSel, true);
        if (checkRet(retCode, "KSJ_SetData16Bits"))
            return false;
    }

    retCode = KSJ_CaptureGetSizeExEx(m_deviceCurSel, &m_SensorResX, &m_SensorResY,
                                     &m_imageBitCount, &m_bitsPerSample);
    if (checkRet(retCode, "KSJ_CaptureGetSizeExEx"))
        return false;

    isSupport = 0;
    retCode = KSJ_QueryFunction(m_deviceCurSel, KSJ_SUPPORT_TRIGGER_MODE_SOFTWARE, &isSupport);
    if (!checkRet(retCode, "KSJ_QueryFunction(KSJ_SUPPORT_TRIGGER_MODE_SOFTWARE)") && isSupport) {
        retCode = KSJ_TriggerModeSet(m_deviceCurSel, KSJ_TRIGGER_SOFTWARE);
        checkRet(retCode, "KSJ_TriggerModeSet(KSJ_TRIGGER_SOFTWARE)");
    }

    if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UD205M_SGYY) {
        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_CLAMPLEVEL, 1);
        checkRet(retCode, "KSJ_SetParam(KSJ_CLAMPLEVEL)");

        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_CDSGAIN, 6);
        checkRet(retCode, "KSJ_SetParam(KSJ_CDSGAIN)");

        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_VGAGAIN, 1);
        checkRet(retCode, "KSJ_SetParam(KSJ_VGAGAIN)");
    } else if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500M_MRNN ||
               m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500M_MRYY) {
        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_GAMMA, readGammaFromIni());
        checkRet(retCode, "KSJ_SetParam(KSJ_GAMMA)");

        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_CONTRAST, 10);
        checkRet(retCode, "KSJ_SetParam(KSJ_CONTRAST)");

        int gain = 20;

        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_RED, gain);
        checkRet(retCode, "KSJ_SetParam(KSJ_RED)");
        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_GREEN, gain);
        checkRet(retCode, "KSJ_SetParam(KSJ_GREEN)");
        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_BLUE, gain);
        checkRet(retCode, "KSJ_SetParam(KSJ_BLUE)");
    } else if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500C_MRYY) {
        retCode = KSJ_WhiteBalanceSet(m_deviceCurSel, KSJ_SWB_AUTO_CONITNUOUS);
        checkRet(retCode, "KSJ_WhiteBalanceSet(KSJ_SWB_AUTO_CONITNUOUS)");

        retCode = KSJ_WhiteBalancePresettingSet(m_deviceCurSel, KSJ_CT_5000K);
        checkRet(retCode, "KSJ_WhiteBalancePresettingSet(KSJ_CT_5000K)");

        retCode = KSJ_BayerSetMode(m_deviceCurSel, KSJ_GBRG_BGR24);
        checkRet(retCode, "KSJ_BayerSetMode(KSJ_RGGB_BGR24_FLIP)");

        int redGain, greenGain, blueGain;
        readRGBGainFromIni(redGain, greenGain, blueGain);
        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_RED, redGain);
        checkRet(retCode, "KSJ_SetParam(KSJ_RED)");
        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_GREEN, greenGain);
        checkRet(retCode, "KSJ_SetParam(KSJ_GREEN)");
        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_BLUE, blueGain);
        checkRet(retCode, "KSJ_SetParam(KSJ_BLUE)");
    }

    retCode = KSJ_QueryFunction(m_deviceCurSel, KSJ_SUPPORT_BAD_PIXEL_CORRECTION, &isSupport);
    if (!checkRet(retCode, "KSJ_QueryFunction(KSJ_SUPPORT_BAD_PIXEL_CORRECTION)") && isSupport == 1) {
        retCode = KSJ_BadPixelCorrectionSet(m_deviceCurSel, true, KSJ_THRESHOLD_HIGH);
        checkRet(retCode, "KSJ_BadPixelCorrectionSet");
    }

    if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500C_MRYY) {
        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_FLIP, 1);
        checkRet(retCode, "KSJ_SetParam(KSJ_FLIP)");
    } else {
        retCode = KSJ_SetParam(m_deviceCurSel, KSJ_FLIP, 0);
        checkRet(retCode, "KSJ_SetParam(KSJ_FLIP)");
    }


    m_isCameraOpen = true;
    qInfo("Camera %d opened", m_deviceCurSel);

    return true;
}

bool WzKsjCamera::closeCamera() {
    if (m_isPreviewEnabled) {
        stopPreview();
    }
    if (m_isCameraOpen) {
        m_isCameraOpen = false;
        qInfo("Camera closed");
    }

    return true;
}

bool WzKsjCamera::getLatestFrame(uint8_t **frameAddress) {
    if (!waitEof(5000)) {
        qWarning("waiting for preview data timeout");
        return false;
    }

    *frameAddress = m_previewBuffer;

    return true;
}

bool WzKsjCamera::setupPreview() {
    int retCode = RET_SUCCESS;

    if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500C_MRYY) {
        retCode = KSJ_PreviewSetMode(m_deviceCurSel, PM_RGBDATA);
        if (checkRet(retCode, "KSJ_PreviewSetMode"))
            return false;
    } else {
        retCode = KSJ_PreviewSetMode(m_deviceCurSel, PM_RAWDATA);
        if (checkRet(retCode, "KSJ_PreviewSetMode"))
            return false;
    }

    return true;
}

bool WzKsjCamera::startPreview() {
    QVariant binning = 1;
//    getParam("Binning", binning);
//    if (binning < 0) binning = 1;
//    if (binning > 4) binning = 4;
    m_previewBinning = binning.toInt();

    int retCode = KSJ_PreviewGetSizeEx(m_deviceCurSel, &m_SensorResX, &m_SensorResY, &m_imageBitCount);
    if (checkRet(retCode, "KSJ_PreviewGetSizeEx"))
        return false;

    QVariant exposureMs = 1;
    getParam("ExposureMs", exposureMs);
    uint32_t exposureTime = exposureMs.toUInt();
    checkExposureTime(&exposureTime);

    retCode = KSJ_SetParam(m_deviceCurSel, KSJ_EXPOSURE, exposureTime);
    if (checkRet(retCode, "KSJ_SetParam(KSJ_EXPOSURE)"))
        return false;

    retCode = KSJ_PreviewStart(m_deviceCurSel, true);
    if (checkRet(retCode, "KSJ_PreviewStart"))
        return false;
    else
        qInfo("Start Preview");

    return true;
}

bool WzKsjCamera::stopPreview() {
    m_isPreviewUpdated = false;
    m_EofFlag = false;

    int retCode = KSJ_PreviewStart(m_deviceCurSel, false);
    if (checkRet(retCode, "KSJ_PreviewStart(false)"))
        return false;

    qInfo("Stop Preview");

    return true;
}

void WzKsjCamera::newFrame(unsigned char *pImageData, int nWidth, int nHeight, int nBitCount) {
    {
        QMutexLocker lock(&m_EofMutex);
        uint32_t previewBufferSize = nWidth * nHeight * (nBitCount >> 3);
        if (m_previewBuffer != nullptr && previewBufferSize != m_previewBufferSize) {
            delete []m_previewBuffer;
            m_previewBuffer = nullptr;
        }
        if (nullptr == m_previewBuffer) {
            m_previewBuffer = new uint8_t[previewBufferSize];
            m_imageBitCount = nBitCount;
            m_previewBufferSize = previewBufferSize;
        }
        memcpy(m_previewBuffer, pImageData, m_previewBufferSize);
        m_imageSize->setWidth(nWidth);
        m_imageSize->setHeight(nHeight);
        m_imageBytes = m_previewBufferSize;

        m_EofFlag = true;
    }
    m_EofCond.notify_one();
}

bool WzKsjCamera::checkRet(int retCode, const QString msg) {
    if (retCode == RET_SUCCESS)
        return false;
    TCHAR szErrorInfo[256] = {0};
    KSJ_GetErrorInfo(retCode, szErrorInfo);
    QString errorInfo = QString::fromWCharArray(szErrorInfo);
    qWarning() << msg << ", Error code: " << retCode << ", " << errorInfo;
    m_latestErrorMsg = QString("%1, Error code: %2, Error message: %3").
            arg(msg).arg(retCode).arg(szErrorInfo);
    return true;
}

bool WzKsjCamera::singleCaptureSetup() {
    // TODO 此函数中通用的参数移动到 openCamera 中
    QVariant binning = 1;
//    getParam("Binning", binning);
//    if (binning < 1)
//        binning = 1;
//    else if (binning > 4)
//        binning = 4;

    m_imageSize->setWidth(m_SensorResX / binning.toInt());
    m_imageSize->setHeight(m_SensorResY / binning.toInt());

    // 灰度累积缓冲区处理
    // 处理灰度累积
    QVariant isGrayAccumulate = false;
    if (getParam("grayAccumulate", isGrayAccumulate) && isGrayAccumulate == true) {
        if (nullptr != m_grayAccumulateBuffer) {
            delete []m_grayAccumulateBuffer;
            m_grayAccumulateBuffer = nullptr;
        }
        m_grayAccumulateBuffer = new uint16_t[m_imageSize->width() * m_imageSize->height()];
        memset(m_grayAccumulateBuffer, 0, m_imageSize->width() * m_imageSize->height() * 2);
    }

    return true;
}

bool WzKsjCamera::singleCaptureStart(uint32_t exposureMs) {
    if (exposureMs < 1)
        exposureMs = 1;
    checkExposureTime(&exposureMs);

    QVariant binning = 1;
//    getParam("Binning", binning);
//    if (binning < 1)
//        binning = 1;
//    else if (binning > 4)
//        binning = 4;

    uint32_t imageBytes = 0;
    int imageWidth = 0, imageHeight = 0;
    int retCode = KSJ_CaptureGetSizeExEx(m_deviceCurSel, &imageWidth, &imageHeight, &m_imageBitCount, &m_bitsPerSample);
    if (checkRet(retCode, "KSJ_CaptureGetSizeEx"))
        return false;

    imageBytes = static_cast<uint32>(imageWidth * imageHeight * (m_imageBitCount >> 3) * (m_bitsPerSample >> 3));
    m_imageSize->setWidth(imageWidth);
    m_imageSize->setHeight(imageHeight);

    qInfo("Capture, exposure: %d, binning: %d, imageWidth: %d, imageHeight: %d, imageBytes: %d",
          exposureMs, binning.toInt(), m_imageSize->width(), m_imageSize->height(), imageBytes);

    if (m_singleFrame != nullptr && m_imageBytes != imageBytes) {
        delete[] m_singleFrame;
        m_singleFrame = nullptr;
    }
    m_imageBytes = imageBytes;
    if (nullptr == m_singleFrame) {
        m_singleFrame = new (std::nothrow) uint8_t[m_imageBytes];
        if (m_singleFrame == nullptr) {
            qWarning("Unable to allocate memory");
            return false;
        }
    }

    retCode = KSJ_CaptureSetTimeOutEx(m_deviceCurSel, 0xFFFFFFFF, true);
    if (checkRet(retCode, "KSJ_CaptureSetTimeOut"))
        return false;

    retCode = KSJ_SetParam(m_deviceCurSel, KSJ_EXPOSURE, exposureMs);
    if (checkRet(retCode, "KSJ_SetParam(KSJ_EXPOSURE)"))
        return false;

    for (int i = 0; i < 2; i++) {
        QElapsedTimer et;
        et.start();
        if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500C_MRYY)
            retCode = KSJ_CaptureRgbData(m_deviceCurSel, reinterpret_cast<unsigned char*>(m_singleFrame));
        else {
            retCode = KSJ_EmptyFrameBuffer(m_deviceCurSel);
            checkRet(retCode, "KSJ_EmptyFrameBuffer");
            retCode = KSJ_CaptureRawData(m_deviceCurSel, reinterpret_cast<unsigned char*>(m_singleFrame));
        }
        if (checkRet(retCode, "KSJ_CaptureRawData"))
            return false;
        // 界面中限制了最多曝光时间为10秒钟, KSJ UC500M 的最大曝光时间是7秒,
        // 所以即使设置了10秒也会在7秒返回, 因此就不用重复拍了
        if (et.elapsed() > exposureMs || et.elapsed() > 7000) {
            break;
        } else {
        }
    }

    qInfo("Capture success.");

    return true;
}

bool WzKsjCamera::singleCaptureAbort() {
    qInfo("Abort the single capture");
    return true;
}

bool WzKsjCamera::waitEof(uint milliseconds) {
    QMutexLocker lock(&m_EofMutex);
    if (!m_EofFlag)
        m_EofCond.wait(&m_EofMutex, milliseconds);
    if (!m_EofFlag) {
        return false;
    }
    m_EofFlag = false;
    return true;
}

bool WzKsjCamera::singleCaptureFinished() {
    return true;
}

bool WzKsjCamera::copyPreviewToSingleFrame() {
    // TODO 待完成
    return true;
}

QString WzKsjCamera::getSettingIniFileName()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + kIniFileName;
}

int WzKsjCamera::readGammaFromIni()
{
    QSettings settings(getSettingIniFileName(), QSettings::IniFormat);
    return settings.value("Camera/gama", 0).toInt();
}

void WzKsjCamera::readRGBGainFromIni(int &redGain, int &greenGain, int &blueGain)
{
    QSettings settings(getSettingIniFileName(), QSettings::IniFormat);
    redGain = settings.value("Camera/redGain", 6).toInt();
    greenGain = settings.value("Camera/greenGain", 6).toInt();
    blueGain = settings.value("Camera/blueGain", 6).toInt();
}

void WzKsjCamera::checkExposureTime(uint32_t *exposureMs)
{
    uint32_t maxMs = 0;
    switch (m_deviceInfo[m_deviceCurSel].DeviceType) {
    case KSJ_UC500M_MRNN:
    case KSJ_UC500M_MRYY:
        maxMs = 7000;
        break;
    default:
        maxMs = 0;
    }
    if (maxMs > 0) {
        if (*exposureMs > maxMs)
            *exposureMs = maxMs;
    }
};
