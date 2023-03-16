#include "ShstToupCamera.h"

QList<ShstCameraInstance*> ShstToupCamera::s_instances = QList<ShstCameraInstance*>();

// TODO 监测相机断开事件
// TODO 观察是否需要进行镜头校正

void WINAPI KSJPREVIEWCALLBACK2(unsigned char *pImageData, int nWidth, int nHeight, int nBitCount, void *lpContext)
{
    ShstToupCamera* ctx = static_cast<ShstToupCamera*>(lpContext);
    ctx->newFrame(pImageData, nWidth, nHeight, nBitCount);
}

ShstToupCamera::ShstToupCamera(QObject *parent) :
    WzAbstractCamera(parent),
    m_isConnected(false),
    m_run(false),
    m_cameraState(WzCameraState::Connecting),
    m_isPreviewEnabled(false),
    m_isPreviewUpdated(false),
    m_isCapture(false),
    m_imageSize(new QSize(672, 550))
{
    qDebug() << "ShstToupCamera";

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

ShstToupCamera::~ShstToupCamera()
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

    if (nullptr != m_toupcamImageBuffer) {
        delete [] m_toupcamImageBuffer;
        m_toupcamImageBuffer = nullptr;
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

void ShstToupCamera::connect()
{
    m_cameraState = WzCameraState::Connecting;
    emit cameraState(m_cameraState);
    m_thread->start();
}

void ShstToupCamera::disconnect()
{
    m_run = false;
    m_thread->wait(5000);
    return;
}

void ShstToupCamera::run()
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
        if (openCamera()) {
            m_isConnected = true;
            m_cameraState = WzCameraState::Connected;
            emit cameraState(m_cameraState);
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
                    //stopPreview();
                    Toupcam_put_ExpoTime(m_handle.handle(), exposureMs.toInt() * 1000);
                    //startPreview();
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
}

int ShstToupCamera::setPreviewEnabled(bool enabled)
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

int ShstToupCamera::getImage(void* imageData)
{
    QMutexLocker lock(&m_mutexImageData);
    memcpy(imageData, m_pImageData, static_cast<size_t>(m_imageBytes));
    return ERROR_NONE;
}

int ShstToupCamera::capture(const int exposureMilliseconds)
{
    int ms[] = {exposureMilliseconds};
    return capture(ms, 1);
}

int ShstToupCamera::capture(const int* exposureMilliseconds, const int count)
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

void ShstToupCamera::abortCapture() {
    qDebug() << "ShstToupCamera::abortCapture(), m_isAbortCapture:" << m_isAbortCapture;
    if (m_isAbortCapture)
        return;
    m_isAbortCapture = true;
    m_cameraState = WzCameraState::CaptureAborting;
    emit cameraState(m_cameraState);
    qDebug() << "ShstToupCamera::abortCapture()";
}

void ShstToupCamera::handleTimerFired()
{
    if (m_isPreviewEnabled && m_isPreviewUpdated) {
        emit previewImageUpdated();
        m_isPreviewUpdated = false;
    } else if (m_isCaptureFinished) {
        emit captureFinished();
        m_isCaptureFinished = false;
    }
}

QSize* ShstToupCamera::getImageSize() {
    return m_imageSize;
}

int ShstToupCamera::getExposureMs() {
    return m_exposureMilliseconds[m_currentFrame];
}

int ShstToupCamera::getLeftExposureMs() {
    return m_leftExposureMilliseconds;
}

int ShstToupCamera::getCurrentFrame() {
    return m_currentFrame;
}

int ShstToupCamera::getCapturedCount() {
    return m_capturedCount;
}

int ShstToupCamera::getImageBytes() {
    return static_cast<int>(m_imageBytes);
}

void ShstToupCamera::processImage() {
    WzImageBuffer imageBuffer;
    imageBuffer.buf = m_singleFrame;
    imageBuffer.width = static_cast<uint32_t>(m_imageSize->width());
    imageBuffer.height = static_cast<uint32_t>(m_imageSize->height());
    imageBuffer.bitDepth = 16;
    imageBuffer.samplesPerPixel = 1;
    imageBuffer.bytesCountOfBuf = static_cast<uint32_t>(m_imageBytes);
    imageBuffer.exposureMs = static_cast<uint32_t>(getExposureMs());
    imageBuffer.captureDateTime = QDateTime::currentDateTime();

    setParam("captureDateTime", imageBuffer.captureDateTime);

    // 灰阶位数拉伸和水平翻转图片
    imageBuffer.update();
//    for (uint32_t n = 0; n < imageBuffer.width * imageBuffer.height; n++) {
//        imageBuffer.bit16Array[n] = (imageBuffer.bit16Array[n] + 1) * 16 - 1;
//    }
    uint16_t* flipLineBuffer = new uint16_t[imageBuffer.width];
    for (uint32_t row = 0; row < imageBuffer.height; row++) {
        uint32_t colStart = row * imageBuffer.width;
        //uint32_t colEnd = (row+1) * imageBuffer.width - 1;
        for (uint32_t col = 0; col < imageBuffer.width; col++) {
            flipLineBuffer[col] = (imageBuffer.bit16Array[colStart + (imageBuffer.width - 1 - col)] + 1) * 16 - 1;
        }
        memcpy(&imageBuffer.bit16Array[colStart], flipLineBuffer, imageBuffer.width * 2);
    }
    delete [] flipLineBuffer;

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
    //if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500C_MRYY)
    //    m_latestThumbFile = WzImageService::createThumbRGB(imageBuffer, m_latestImageFile);
    //else
        m_latestThumbFile = WzImageService::createThumb(imageBuffer, m_latestImageFile, isThumbNegative.toBool());

    uint16_t* oldBuffer = imageBuffer.bit16Array;
    int newWidth, newHeight;
    newWidth = imageBuffer.width;
    newHeight = imageBuffer.height;
    qreal zoom = 1;
    /*
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
    */
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


QString ShstToupCamera::getLatestImageFile() {
    return m_latestImageFile;
};

QString ShstToupCamera::getLatestThumbFile() {
    return m_latestThumbFile;
}

double ShstToupCamera::getTemperature() {
    return 0;
}

bool ShstToupCamera::isRgbImage() const
{
    /*
    if (m_deviceInfo[m_deviceCurSel].DeviceType == KSJ_UC500C_MRYY) {
        return true;
    } else {
        return false;
    }
    */
    return false;
}

bool ShstToupCamera::openCamera() {
    if (instances()->count() == 0) {
        qWarning() << "Toup Camera instances are zero";
        return false;
    }
    auto firstInstance = instances()->at(0);
    return openCamera(firstInstance);
}

bool ShstToupCamera::openCamera(ShstCameraInstance* instance) {

    m_activeCameraInstance = qobject_cast<ShstToupCameraInstance*>(instance);
    if (!m_activeCameraInstance) {
        return false;
    }

    m_handle.setHandle(Toupcam_Open(m_activeCameraInstance->toupcamDevice().id));
    if (!m_handle.handle()) {
        qWarning() << "Toupcam opening failure";
        return false;
    }

    uint32_t nMin, nMax, nDef;
    auto hr = Toupcam_get_ExpTimeRange(m_handle.handle(), &nMin, &nMax, &nDef);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_get_ExpTimeRange error:" << hr;
    } else {
        qInfo("Toupcam, minexp: %d, maxexp: %d", nMin, nMax);
    }


    Toupcam_put_AutoExpoEnable(m_handle.handle(), 0);


    /*
    int isSupport = 0;
    retCode = KSJ_QueryFunction(m_deviceCurSel, KSJ_SUPPORT_16BITS, &isSupport);
    if (checkRet(retCode, "KSJ_QueryFunction(KSJ_SUPPORT_16BITS)"))
        return false;
    if (isSupport) {
        retCode = KSJ_SetData16Bits(m_deviceCurSel, true);
        if (checkRet(retCode, "KSJ_SetData16Bits"))
            return false;
    }*/

    // 找到最大的预览分辨率
    auto toupcamModel = m_activeCameraInstance->toupcamDevice().model;
    int indexOfMaxRes = 0;
    uint32_t maxRes = 0;
    for (uint32_t i = 0; i < toupcamModel->preview; i++) {
        auto curRes = toupcamModel->res[i].width * toupcamModel->res[i].height;
        if (curRes > maxRes) {
            maxRes = curRes;
            indexOfMaxRes = i;
        }
    }
    m_SensorResX = toupcamModel->res[indexOfMaxRes].width;
    m_SensorResY = toupcamModel->res[indexOfMaxRes].height;

    uint32_t nFourCC, bitsperpixel;
    hr = Toupcam_get_RawFormat(m_handle.handle(), &nFourCC, &bitsperpixel);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_get_RawFormat error:" << hr;
        return false;
    }
    m_imageBitCount = bitsperpixel;

    /*
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
    */



    m_isCameraOpen = true;
    qInfo() << "Camera opened," << QString::fromWCharArray(m_activeCameraInstance->toupcamDevice().id);

    return true;
}

bool ShstToupCamera::closeCamera() {
    if (m_isPreviewEnabled) {
        stopPreview();
    }
    if (m_isCameraOpen) {
        Toupcam_Close(m_handle.handle());
        m_isCameraOpen = false;
        qInfo("Camera closed");
    }

    return true;
}

bool ShstToupCamera::getLatestFrame(uint8_t **frameAddress) {
    if (!waitEof(5000)) {
        qWarning("waiting for preview data timeout");
        return false;
    }

    *frameAddress = m_previewBuffer;

    return true;
}

bool ShstToupCamera::setupPreview() {
    HRESULT hr = S_OK;

    hr = Toupcam_put_Option(m_handle.handle(), TOUPCAM_OPTION_RAW, 1);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_put_Option(TOUPCAM_OPTION_RAW) error:" << hr;
        return false;
    }

    hr = Toupcam_put_Option(m_handle.handle(), TOUPCAM_OPTION_TRIGGER, 0);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_put_Option(TOUPCAM_OPTION_TRIGGER) error:" << hr;
        return false;
    }

    hr = Toupcam_put_Option(m_handle.handle(), TOUPCAM_OPTION_BITDEPTH, 0);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_put_Option(TOUPCAM_OPTION_BITDEPTH) error:" << hr;
        return false;
    }

    hr = Toupcam_put_Option(m_handle.handle(), TOUPCAM_OPTION_FRAMERATE, 5);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_put_Option(TOUPCAM_OPTION_FRAMERATE) error:" << hr;
        return false;
    }

    uint32_t previewBufferSize = m_SensorResX * m_SensorResY;
    if (m_toupcamImageBuffer != nullptr && previewBufferSize != m_toupcamImageBufferSize) {
        delete []m_toupcamImageBuffer;
        m_toupcamImageBuffer = nullptr;
    }
    if (nullptr == m_toupcamImageBuffer) {
        m_toupcamImageBufferSize = previewBufferSize;
        m_toupcamImageBuffer = new uint8_t[m_toupcamImageBufferSize];
    }

    if (m_previewBuffer != nullptr && previewBufferSize != m_previewBufferSize) {
        delete []m_previewBuffer;
        m_previewBuffer = nullptr;
    }
    if (nullptr == m_previewBuffer) {
        m_previewBufferSize = previewBufferSize;
        m_previewBuffer = new uint8_t[m_previewBufferSize];
    }

    return true;
}

 bool ShstToupCamera::startPreview() {
    QVariant binning = 1;
    //    getParam("Binning", binning);
    //    if (binning < 0) binning = 1;
    //    if (binning > 4) binning = 4;
    m_previewBinning = binning.toInt();

    HRESULT hr = S_OK;
    /*
    int retCode = KSJ_PreviewGetSizeEx(m_deviceCurSel, &m_SensorResX, &m_SensorResY, &m_imageBitCount);
    if (checkRet(retCode, "KSJ_PreviewGetSizeEx"))
        return false;
    */
    // TODO 设置分辨率

    QVariant exposureMs = 1;
    getParam("ExposureMs", exposureMs);
    uint32_t exposureTime = exposureMs.toUInt();
    checkExposureTime(&exposureTime);
    exposureTime = exposureTime * 1000;

    hr = Toupcam_put_ExpoTime(m_handle.handle(), exposureTime);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_put_ExpoTime error:" << hr;
        return false;
    }

    m_isCaptureMode = false;

    hr = Toupcam_StartPullModeWithCallback(m_handle.handle(), eventCallBack, this);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_StartPullModeWithCallback error:" << hr;
        return false;
    }
    qInfo("Start Preview");

    return true;
}

bool ShstToupCamera::stopPreview() {
    m_isPreviewUpdated = false;
    m_EofFlag = false;

    HRESULT hr = Toupcam_Stop(m_handle.handle());
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_Stop error:" << hr;
        return false;
    }

    qInfo("Stopped Preview");
    return true;
}

void ShstToupCamera::newFrame(unsigned char *pImageData, int nWidth, int nHeight, int nBitCount) {
    Q_UNUSED(nBitCount)
    {
        QMutexLocker lock(&m_EofMutex);

        if (m_isCaptureMode)
            memcpy(m_singleFrame, pImageData, m_imageBytes);
        else {
            m_imageBytes = m_previewBufferSize;
            memcpy(m_previewBuffer, pImageData, m_previewBufferSize);
        }
        m_imageSize->setWidth(nWidth);
        m_imageSize->setHeight(nHeight);

        m_EofFlag = true;
    }
    m_EofCond.notify_one();
}

int ShstToupCamera::count()
{
    for (int i = 0; i < s_instances.count(); i++)
        delete s_instances.at(i);
    s_instances.clear();

    ToupcamDeviceV2 pti[TOUPCAM_MAX];
    uint32_t n = Toupcam_EnumV2(pti);
    for (uint32_t i = 0 ; i < n; i++) {
        ShstToupCameraInstance* instance = new ShstToupCameraInstance();
        instance->setModel(QString::fromWCharArray(pti[i].model->name));
        instance->setToupcamDevice(pti[i]);
        s_instances.append(instance);
        qDebug() << (*instance);
    };
    return n;
}

QList<ShstCameraInstance*>* ShstToupCamera::instances()
{
    return &s_instances;
}

bool ShstToupCamera::checkRet(int retCode, const QString msg) {
    return false;
    /*
    if (retCode == RET_SUCCESS)
        return false;
    TCHAR szErrorInfo[256] = {0};
    KSJ_GetErrorInfo(retCode, szErrorInfo);
    QString errorInfo = QString::fromWCharArray(szErrorInfo);
    qWarning() << msg << ", Error code: " << retCode << ", " << errorInfo;
    m_latestErrorMsg = QString("%1, Error code: %2, Error message: %3").
                       arg(msg).arg(retCode).arg(szErrorInfo);
    return true;
    */
}

bool ShstToupCamera::singleCaptureSetup() {
    // TODO 此函数中通用的参数移动到 openCamera 中
    QVariant binning = 1;
    //    getParam("Binning", binning);
    //    if (binning < 1)
    //        binning = 1;
    //    else if (binning > 4)
    //        binning = 4;

    m_imageSize->setWidth(m_SensorResX / binning.toInt());
    m_imageSize->setHeight(m_SensorResY / binning.toInt());

    HRESULT hr = Toupcam_put_Option(m_handle.handle(), TOUPCAM_OPTION_RAW, 1);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_put_Option(TOUPCAM_OPTION_RAW) error:" << hr;
        return false;
    }

    hr = Toupcam_put_Option(m_handle.handle(), TOUPCAM_OPTION_TRIGGER, 1);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_put_Option(TOUPCAM_OPTION_TRIGGER) error:" << hr;
        return false;
    }

    hr = Toupcam_put_Option(m_handle.handle(), TOUPCAM_OPTION_BITDEPTH, 1);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_put_Option(TOUPCAM_OPTION_BITDEPTH) error:" << hr;
        return false;
    }

    //qDebug() << Toupcam_get_MaxBitDepth(m_handle.handle());

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

bool ShstToupCamera::singleCaptureStart(uint32_t exposureMs) {
    if (exposureMs < 1)
        exposureMs = 1;
    checkExposureTime(&exposureMs);

    HRESULT hr = S_OK;
//    HRESULT hr = Toupcam_Stop(m_handle.handle());
//    if (!SUCCEEDED(hr)) {
//        qWarning() << "Toupcam_Stop error:" << hr;
//    }

    hr = Toupcam_put_ExpoTime(m_handle.handle(), exposureMs * 1000);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_put_ExpoTime error:" << hr;
        return false;
    }

    //QVariant binning = 1;
    //    getParam("Binning", binning);
    //    if (binning < 1)
    //        binning = 1;
    //    else if (binning > 4)
    //        binning = 4;

    // 找到最大的拍摄分辨率
    uint32_t imageBytes = 0;
    int imageWidth = m_SensorResX, imageHeight = m_SensorResY;

    auto bitDepth = Toupcam_get_MaxBitDepth(m_handle.handle());
    imageBytes = static_cast<uint32>(imageWidth * imageHeight * (bitDepth > 8 ? 2 : 1));
    m_imageSize->setWidth(imageWidth);
    m_imageSize->setHeight(imageHeight);

    if (m_toupcamImageBuffer != nullptr && imageBytes != m_toupcamImageBufferSize) {
        delete []m_toupcamImageBuffer;
        m_toupcamImageBuffer = nullptr;
    }
    if (nullptr == m_toupcamImageBuffer) {
        m_toupcamImageBufferSize = imageBytes;
        m_toupcamImageBuffer = new uint8_t[m_toupcamImageBufferSize];
    }

    qInfo("Capture, exposure: %d, imageWidth: %d, imageHeight: %d, imageBytes: %d",
          exposureMs, m_imageSize->width(), m_imageSize->height(), imageBytes);

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

    hr = Toupcam_StartPullModeWithCallback(m_handle.handle(), eventCallBack, this);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_StartPullModeWithCallback error:" << hr;
        return false;
    }

    m_isCaptureMode = true;

    hr = Toupcam_Trigger(m_handle.handle(), 1);
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_Trigger error:" << hr;
        return false;
    }

    // 此函数在多线程被调用, 所以这里可以长时间循环等待拿到图片之后返回
    int waitMs = exposureMs * 2;
    while (waitMs > 0) {
        if (waitEof(500)) {
            break;
        } else {
            waitMs = waitMs - 500;
        }
    }

    hr = Toupcam_Stop(m_handle.handle());
    if (!SUCCEEDED(hr)) {
        qWarning() << "Toupcam_Stop error:" << hr;
    }

    qInfo("Capture success.");

    return true;
}

bool ShstToupCamera::singleCaptureAbort() {
    qInfo("Abort the single capture");
    return true;
}

bool ShstToupCamera::waitEof(uint milliseconds) {
    QMutexLocker lock(&m_EofMutex);
    if (!m_EofFlag)
        m_EofCond.wait(&m_EofMutex, milliseconds);
    if (!m_EofFlag) {
        return false;
    }
    m_EofFlag = false;
    return true;
}

bool ShstToupCamera::singleCaptureFinished() {
    return true;
}

bool ShstToupCamera::copyPreviewToSingleFrame() {
    // TODO 待完成
    return true;
}

QString ShstToupCamera::getSettingIniFileName()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + kIniFileName;
}

int ShstToupCamera::readGammaFromIni()
{
    QSettings settings(getSettingIniFileName(), QSettings::IniFormat);
    return settings.value("Camera/gama", 0).toInt();
}

void ShstToupCamera::readRGBGainFromIni(int &redGain, int &greenGain, int &blueGain)
{
    QSettings settings(getSettingIniFileName(), QSettings::IniFormat);
    redGain = settings.value("Camera/redGain", 6).toInt();
    greenGain = settings.value("Camera/greenGain", 6).toInt();
    blueGain = settings.value("Camera/blueGain", 6).toInt();
}

void ShstToupCamera::checkExposureTime(uint32_t *exposureMs)
{
    unsigned int nMin, nMax, nDef;
    Toupcam_get_ExpTimeRange(m_handle.handle(), &nMin, &nMax, &nDef);
    if (*exposureMs > nMax / 1000)
        *exposureMs = nMax / 1000;
    else if (*exposureMs < nMin / 1000)
        *exposureMs = nMin / 1000;
}

void ShstToupCamera::eventCallBack(unsigned nEvent, void *pCallbackCtx)
{
    ShstToupCamera* pThis = static_cast<ShstToupCamera*>(pCallbackCtx);
    if (TOUPCAM_EVENT_IMAGE == nEvent)
        pThis->handleImageCallback();
    else if (TOUPCAM_EVENT_EXPOSURE == nEvent)
        pThis->handleExpCallback();
}

void ShstToupCamera::handleImageCallback()
{
    unsigned width = 0, height = 0;
    HRESULT hr = Toupcam_PullImage(m_handle.handle(), m_toupcamImageBuffer, 0, &width, &height);
    if (SUCCEEDED(hr)) {
        newFrame(m_toupcamImageBuffer, width, height, m_imageBitCount);
    } else {
        qWarning() << "Toupcam_PullImage error:" << hr;
    }
}

void ShstToupCamera::handleExpCallback()
{

};

ShstToupCameraInstance::ShstToupCameraInstance(QObject *parent)
    : ShstCameraInstance{parent} {

}

QString ShstToupCameraInstance::getName() const
{
    return m_name;
}

QString ShstToupCameraInstance::getDisplayName() const
{
    return m_displayName;
}

QString ShstToupCameraInstance::getModel() const
{
    return m_model;
}

void ShstToupCameraInstance::setModel(const QString &newModel)
{
    m_model = newModel;
}

ShstToupCameraInstance::operator QString() const
{
    return QString("%1, %2, %3")
        .arg(QString::fromWCharArray(m_toupcamDevice.displayname),
             QString::fromWCharArray(m_toupcamDevice.id),
             QString::fromWCharArray(m_toupcamDevice.model->name));
}

void ShstToupCameraInstance::setName(const QString &newName)
{
    m_name = newName;
}

void ShstToupCameraInstance::setDisplayName(const QString &newDisplayName)
{
    m_displayName = newDisplayName;
}

const ToupcamDeviceV2 &ShstToupCameraInstance::toupcamDevice() const
{
    return m_toupcamDevice;
}

void ShstToupCameraInstance::setToupcamDevice(const ToupcamDeviceV2 &newToupcamDevice)
{
    m_toupcamDevice = newToupcamDevice;
}
