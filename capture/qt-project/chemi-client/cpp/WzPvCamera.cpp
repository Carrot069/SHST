#include "WzPvCamera.h"

// TODO 监测相机断开事件
// TODO 相机温度还没到-30时，开启了预览，温度不能获取的问题，准备使用定时器模拟相机降温过程
// TODO 滤波降噪
// TODO 观察是否需要进行镜头校正

// Function that gets called from PVCAM when EOF event arrives
void NewFrameHandler(FRAME_INFO *pFrameInfo, void *context)
{
    Q_UNUSED(pFrameInfo)
    WzPvCamera* ctx = static_cast<WzPvCamera*>(context);
    ctx->newFrame();
}

WzPvCamera::WzPvCamera(QObject *parent) :
    WzAbstractCamera(parent),
    m_isConnected(false),
    m_run(false),
    m_cameraState(WzCameraState::Connecting),
    m_isPreviewEnabled(false),
    m_isPreviewUpdated(false),
    m_isCapture(false),
    m_imageSize(new QSize(672, 550))
{
    qDebug() << "WzPvCamera";

    m_cameraParameters["ExposureMs"] = 10;
    m_cameraParameters["Binning"   ] = 4;

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

WzPvCamera::~WzPvCamera()
{
    disconnect();
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

void WzPvCamera::connect()
{
    m_cameraState = WzCameraState::Connecting;
    emit cameraState(m_cameraState);
    m_thread->start();
}

void WzPvCamera::disconnect()
{
    m_run = false;
    m_thread->wait(5000);
    return;
}

void WzPvCamera::run()
{
    int temperatureTick = 0;
    bool isStartedCapture = false;
    bool isStartedExposure = false;
    bool isStartedPreview = false;
    bool isAutoExposureFinsied = false;
    QThread::msleep(1000);
    m_isConnected = false;
    m_run = true;
    m_cameraState = WzCameraState::Connecting;
    emit cameraState(m_cameraState);

    QRandomGenerator rg;

    if (nullptr == m_cameraCallback) {
        qWarning("nullptr == m_cameraCallback");
    } else {
        // TODO
        if (initPVCAM()) {
            if (openCamera()) {
                if (m_cameraCallback->cameraSN(QString(m_camSerNum))) {
                    setTemperature(-30);
                    m_isConnected = true;
                    m_cameraState = WzCameraState::Connected;
                    emit cameraState(m_cameraState);
                } else {
                    closeCamera();
                    uninitPVCAM();
                }
            }
        } else {
            m_cameraState = WzCameraState::CameraNotFound;
            emit cameraState(m_cameraState);
            uninitPVCAM();
        }
    }

    while (m_run) {
        if (m_isCameraOpen == FALSE) {
            QThread::msleep(10);
            continue;
        }
        if (m_isPreviewEnabled) {
            if (!isStartedPreview) {
                if (setupPreview() && startPreview()) {
                    QVariant exposureMs = 1;
                    getParam("ExposureMs", exposureMs);
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
                uns16 *frameBytes = nullptr;
                if (getLatestFrame(&frameBytes)) {
                    QMutexLocker lock(&m_mutexImageData);

                    uchar* pDest = m_previewImage8bit;
                    uint16_t* pSrc = frameBytes;

                    WzUtils::calc16bitTo8bit(pSrc,
                                             static_cast<uint32_t>(m_imageSize->width() * m_imageSize->height()),
                                             m_gray16to8Table,
                                             4000);
                    for (int row = 0; row < m_imageSize->height(); row++) {
                        for (int col = 0; col < m_imageSize->width(); col++) {
                            *pDest = m_gray16to8Table[*pSrc];
                            pSrc++;
                            pDest++;
                        }
                    }

                    // 如果连续100个像素都是0灰阶说明可能没开光源, 所以不把当前画面复制到16位缓冲区
                    // 此缓冲区内的图片是为了保存为Marker图用, 所以必须有内容
                    if (pSrc[m_imageSize->width() * 100 + 100] == 0) {
                        int start = m_imageSize->width() * 100 + 100;
                        uint32_t grays = 0;
                        for (int i = start; i < start + 100; i++) {
                            grays += pSrc[i];
                        }
                        if (grays > 0) {
                            memcpy(m_previewImage16bit, pSrc,
                                   m_previewImage16bitLength * sizeof(m_previewImage16bit[0]));
                        }
                    }
                    m_pImageData = m_previewImage8bit;
                    memcpy(m_previewImage16bit, pSrc,
                           m_previewImage16bitLength * sizeof(m_previewImage16bit[0]));
                    m_isPreviewUpdated = true;
                }

                if (m_resetPreview) {
                    m_resetPreview = false;
                    stopPreview();
                    updateTemperature();
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
                    // TODO 停止预览失败了
                }
            }
        }
        if (m_isCapture) {
            if (m_cameraState == WzCameraState::PreviewStarted) {
                m_isPreviewEnabled = false;
                isStartedPreview = false;
                m_cameraState = WzCameraState::PreviewStopping;
                emit cameraState(m_cameraState);
                if (stopPreview()) {
                    m_cameraState = WzCameraState::PreviewStopped;
                    emit cameraState(m_cameraState);
                } else {
                    // TODO 处理停止失败
                }
            }
            // 拍摄前的初始化
            if (!isStartedCapture) {
                m_cameraState = WzCameraState::CaptureInit;
                emit cameraState(m_cameraState);

                QThread::msleep(500); // wait for light closed
                singleCaptureSetup();
                m_currentFrame = 0;
                m_capturedCount = 0;
                m_leftExposureMilliseconds = m_exposureMilliseconds[m_currentFrame];

                isStartedCapture = true;
            }

            QVariant isAutoExposure = false;
            getParam("AE", isAutoExposure);
            if (isAutoExposure.toBool() && !isAutoExposureFinsied) {
                m_cameraState = WzCameraState::AutoExposure;
                emit cameraState(m_cameraState);

                QVariant binning = 1;
                getParam("Binning", binning);
                if (binning < 0) binning = 1;
                if (binning > 4) binning = 4;

                isAutoExposureFinsied = true;
                int aeMs = autoExposure(1, binning.toInt());
                qInfo("AE ms: %d", aeMs);
                if (aeMs > 0) {
                    m_exposureMilliseconds[0] = aeMs;
                    m_leftExposureMilliseconds = m_exposureMilliseconds[0];
                }
                m_cameraState = WzCameraState::AutoExposureFinished;
                emit cameraState(m_cameraState);
                singleCaptureSetup();
            }

            if (m_isAbortCapture) {
                qInfo("Check abort single capture in thread");
                m_isAbortCapture = false;
                m_isCapture = false;
                isStartedCapture = false;
                isAutoExposureFinsied = false;
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
                m_cameraState = WzCameraState::Exposure;
                emit cameraState(m_cameraState);
                // TODO 考虑启动曝光失败了如何处理
                if (singleCaptureStart(static_cast<uns32>(m_exposureMilliseconds[m_currentFrame]))) {
                    isStartedExposure = true;
                }
            }

            m_leftExposureMilliseconds -= 10;

            if (m_leftExposureMilliseconds <= 0) {
                m_leftExposureMilliseconds = 0;
                // 全部拍完了
                if (m_currentFrame+1 == m_frameCount) {
                    m_isCapture = false;
                    m_isCaptureFinished = true;                    
                    isStartedCapture = false;
                    isStartedExposure = false;
                    isAutoExposureFinsied = false;
                }

                m_cameraState = WzCameraState::Image;
                emit cameraState(m_cameraState);

                // 总共等待20秒, 在小间隔时间内监测是不是取消了拍摄
                bool isTimeout = true;
                for (int n = 0; n < 40; n++) {
                    if (m_isAbortCapture) {
                        break;
                    }
                    if (waitEof(500)) {
                        isTimeout = false;
                        break;
                    } else {
                        qInfo("Waiting for the Eof flag, %d", n * 500);
                    }
                }
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
                if (isTimeout) {
                    // TODO 处理超时后的情况
                } else {
                    qInfo("Exposure finished");
                }

                isStartedExposure = false;
                singleCaptureFinished();

                {
                    QMutexLocker lock(&m_mutexImageData);
                    m_pImageData = m_singleFrame;
                    processImage();
                }

                m_capturedCount++;
                qDebug("m_capturedCount: %d", m_capturedCount);
                m_cameraState = WzCameraState::CaptureFinished;
                emit cameraState(m_cameraState);
                QThread::msleep(200); // 必须停一下, 防止上层逻辑检测不到这个状态就变掉了

                if (m_currentFrame+1 < m_frameCount) {
                    m_currentFrame++;
                    m_leftExposureMilliseconds = m_exposureMilliseconds[m_currentFrame];
                }
            }
        }

        if (!m_isPreviewEnabled && !m_isCapture) {
            temperatureTick++;
            // It's updated every second
            if (temperatureTick >= 100) {
                updateTemperature();
                temperatureTick = 0;
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
    uninitPVCAM();
}

int WzPvCamera::setPreviewEnabled(bool enabled)
{
    if (enabled) {
        m_cameraState = WzCameraState::PreviewStarting;
        emit cameraState(m_cameraState);
        qDebug() << "setPreviewEnabled, enabled=" << enabled;
    } else {
        m_cameraState = WzCameraState::PreviewStopping;
        emit cameraState(m_cameraState);
    }
    m_isPreviewEnabled = enabled;
    return ERROR_NONE;
}

void WzPvCamera::resetPreview()
{
    m_resetPreview = true;
}

int WzPvCamera::getImage(void* imageData)
{
    QMutexLocker lock(&m_mutexImageData);
    memcpy(imageData, m_pImageData, static_cast<size_t>(m_imageBytes));
    return ERROR_NONE;
}

int WzPvCamera::capture(const int exposureMilliseconds)
{
    int ms[] = {exposureMilliseconds};
    return capture(ms, 1);
}

int WzPvCamera::capture(const int* exposureMilliseconds, const int count)
{
    if (count == 0) return ERROR_PARAM;
    m_frameCount = count;
    if (m_exposureMilliseconds)
        delete[] m_exposureMilliseconds;
    m_exposureMilliseconds = new int[count];
    memcpy(m_exposureMilliseconds, exposureMilliseconds, static_cast<uint>(count) * sizeof(int));
    m_isAbortCapture = false;
    m_isCapture = true;
    qDebug() << "WzPvCamera::capture";
    return ERROR_NONE;
}

void WzPvCamera::abortCapture() {
    qDebug() << "WzPvCamera::abortCapture(), m_isAbortCapture:" << m_isAbortCapture;
    if (m_isAbortCapture)
        return;
    m_isAbortCapture = true;
    m_cameraState = WzCameraState::CaptureAborting;
    emit cameraState(m_cameraState);
    qDebug() << "WzPvCamera::abortCapture()";
}

void WzPvCamera::handleTimerFired()
{
    if (m_isPreviewEnabled && m_isPreviewUpdated) {
        emit previewImageUpdated();
        m_isPreviewUpdated = false;
    } else if (m_isCaptureFinished) {
        emit captureFinished();
        m_isCaptureFinished = false;
    }
}

QSize* WzPvCamera::getImageSize() {
    return m_imageSize;
}

int WzPvCamera::getExposureMs() {
    if (nullptr == m_exposureMilliseconds)
        return 1;
    return m_exposureMilliseconds[m_currentFrame];
}

int WzPvCamera::getLeftExposureMs() {
    return m_leftExposureMilliseconds;
}

int WzPvCamera::getCurrentFrame() {
    return m_currentFrame;
}

int WzPvCamera::getCapturedCount() {
    return m_capturedCount;
}

int WzPvCamera::getImageBytes() {
    return static_cast<int>(m_imageBytes);
}

void WzPvCamera::processImage() {
    WzImageBuffer imageBuffer;
    imageBuffer.buf = reinterpret_cast<uint8_t*>(m_singleFrame);
    imageBuffer.width = static_cast<uint32_t>(m_imageSize->width());
    imageBuffer.height = static_cast<uint32_t>(m_imageSize->height());
    imageBuffer.bitDepth = 16;
    imageBuffer.bytesCountOfBuf = static_cast<uint32_t>(m_imageBytes);
    imageBuffer.exposureMs = getExposureMs();
    imageBuffer.captureDateTime = QDateTime::currentDateTime();

    setParam("captureDateTime", imageBuffer.captureDateTime);
    setParam("imageWidth", imageBuffer.width);
    setParam("imageHeight", imageBuffer.height);

    // 处理灰度累积
    QVariant isGrayAccumulate = false;
    if (getParam("grayAccumulate", isGrayAccumulate) && isGrayAccumulate == true) {
        if (nullptr == m_grayAccumulateBuffer) {
            qWarning("m_grayAccumulateBuffer == nullptr");
        } else {
            for (uint i = 0; i < imageBuffer.width * imageBuffer.height; i++) {
                int newGray = m_singleFrame[i] + m_grayAccumulateBuffer[i];
                if (newGray > 65535)
                    newGray = 65535;
                m_singleFrame[i] = newGray;
                m_grayAccumulateBuffer[i] = newGray;
            }
        }
        QVariant grayAccumulateAddExposure = false;
        if (getParam("grayAccumulateAddExposure", grayAccumulateAddExposure) &&
                grayAccumulateAddExposure == true) {
            imageBuffer.exposureMs = imageBuffer.exposureMs * (m_currentFrame + 1);
        }
    }

    // TODO 降噪、畸变校正、处理背景
    WzImageFilter filter;
    filter.filterLightspot(m_singleFrame, imageBuffer.width, imageBuffer.height);

    QString path = mkdirImagePath();
    m_latestImageFile = getTiffFileName(path, imageBuffer.captureDateTime);
    WzImageService::saveImageAsTiff(imageBuffer, m_latestImageFile);

    imageBuffer.update();
    QVariant isThumbNegative = true;
    getParam("isThumbNegative", isThumbNegative);
    m_latestThumbFile = WzImageService::createThumb(imageBuffer, m_latestImageFile, isThumbNegative.toBool());

    // get min/max gray from marker
    if (nullptr != m_previewImage16bit) {
        int minGray = m_previewImage16bit[0];
        int maxGray = m_previewImage16bit[0];
        for (int i = 0; i < m_previewImage16bitLength; i++) {
            if (minGray > m_previewImage16bit[i])
                minGray = m_previewImage16bit[i];
            if (maxGray < m_previewImage16bit[i])
                maxGray = m_previewImage16bit[i];
        }
        if (maxGray < 4000)
            maxGray = 4000;
        setParam("MarkerMinGray", minGray);
        setParam("MarkerMaxGray", maxGray);
    }
}


QString WzPvCamera::getLatestImageFile() {
    return m_latestImageFile;
};

QString WzPvCamera::getLatestThumbFile() {
    return m_latestThumbFile;
}

bool WzPvCamera::initPVCAM() {
    if (PV_OK != pl_pvcam_init()) {
        printErrorMessage(pl_error_code(), "pl_pvcam_init() error");
        return false;
    }
    m_isPvcamInitialized = TRUE;
    qInfo("PVCAM initialized");

    if (PV_OK != pl_cam_get_total(&m_numberOfCameras)) {
        printErrorMessage(pl_error_code(), "pl_cam_get_total() error");
        return false;
    }

    if (m_numberOfCameras == 0) {
        qInfo("No cameras found in the system");
        return false;
    }
    qInfo("Number of cameras found: %d", m_numberOfCameras);

    return true;
}

bool WzPvCamera::uninitPVCAM() {
    // Uninitialize PVCAM library
    if (m_isPvcamInitialized == TRUE) {
        if (PV_OK != pl_pvcam_uninit()) {
            printErrorMessage(pl_error_code(), "pl_pvcam_uninit() error");
            return false;
        } else {
            m_isPvcamInitialized = FALSE;
        }
        qInfo("PVCAM uninitializated");
    }

    return true;
}

bool WzPvCamera::openCamera() {
    // TODO 大于0时用Mutex的形式等待UI弹出窗口中选择相机后再连接
    QStringList cameraNames;
    char cameraName[CAM_NAME_LEN];
    for (int16 n = 0; n < m_numberOfCameras; n++) {
        if (PV_OK != pl_cam_get_name(n, cameraName)) {
            printErrorMessage(pl_error_code(), "pl_cam_get_name() error");
            return false;
        }
        cameraNames.append(cameraName);
    }

    if (PV_OK != pl_cam_open(cameraName, &m_hCam, OPEN_EXCLUSIVE)) {
        printErrorMessage(pl_error_code(), "pl_cam_open() error");
        return false;
    }
    m_isCameraOpen = TRUE;
    qInfo("Camera %s opened", cameraName);

    // Read the serial number of Camera
    if (!isParamAvailable(PARAM_HEAD_SER_NUM_ALPHA, "PARAM_HEAD_SER_NUM_ALPHA"))
        return false;
    if (PV_OK != pl_get_param(m_hCam, PARAM_HEAD_SER_NUM_ALPHA, ATTR_CURRENT,
                              reinterpret_cast<void*>(m_camSerNum))) {
        printErrorMessage(pl_error_code(), "pl_get_param(PARAM_HEAD_SER_NUM_ALPHA) error");
        return false;
    }
    qInfo("Camera number: %s", m_camSerNum);

    // Read the version of Device Driver
    if (!isParamAvailable(PARAM_DD_VERSION, "PARAM_DD_VERSION"))
        return false;
    uns16 ddVersion;
    if (PV_OK != pl_get_param(m_hCam, PARAM_DD_VERSION, ATTR_CURRENT,
                              reinterpret_cast<void*>(&ddVersion))) {
        printErrorMessage(pl_error_code(), "pl_get_param(PARAM_DD_VERSION) error");
        return false;
    }
    qInfo("Device driver version: %d.%d.%d",
            (ddVersion >> 8) & 0xFF,
            (ddVersion >> 4) & 0x0F,
            (ddVersion >> 0) & 0x0F);

    // Get camera chip name string. Typically holds both chip and camera model
    // name, therefore is the best camera identifier for most models
    if (!isParamAvailable(PARAM_CHIP_NAME, "PARAM_CHIP_NAME"))
        return false;
    char chipName[CCD_NAME_LEN];
    if (PV_OK != pl_get_param(m_hCam, PARAM_CHIP_NAME, ATTR_CURRENT,
                              reinterpret_cast<void*>(chipName)))
    {
        printErrorMessage(pl_error_code(), "pl_get_param(PARAM_CHIP_NAME) error");
        return false;
    }
    qInfo("Sensor chip name: %s", chipName);

    // Get camera firmware version
    if (!isParamAvailable(PARAM_CAM_FW_VERSION, "PARAM_CAM_FW_VERSION"))
        return false;
    uns16 fwVersion;
    if (PV_OK != pl_get_param(m_hCam, PARAM_CAM_FW_VERSION, ATTR_CURRENT,
                              reinterpret_cast<void*>(&fwVersion))) {
        printErrorMessage(pl_error_code(),
                "pl_get_param(PARAM_CAM_FW_VERSION) error");
        return false;
    }
    qInfo("Camera firmware version: %d.%d",
          (fwVersion >> 8) & 0xFF,
          (fwVersion >> 0) & 0xFF);

    // Find out if the sensor is a frame transfer or other (typically interline)
    // type. This is a two-step process.
    // Please, follow the procedure below in your applications.
    if (PV_OK != pl_get_param(m_hCam, PARAM_FRAME_CAPABLE, ATTR_AVAIL,
                              reinterpret_cast<void*>(&m_isFrameTransfer))) {
        m_isFrameTransfer = 0;
        printErrorMessage(pl_error_code(),
                "pl_get_param(PARAM_FRAME_CAPABLE) error");
        return false;
    }

    if (m_isFrameTransfer == TRUE) {
        if (PV_OK != pl_get_param(m_hCam, PARAM_FRAME_CAPABLE, ATTR_CURRENT,
                                  reinterpret_cast<void*>(&m_isFrameTransfer))) {
            m_isFrameTransfer = 0;
            printErrorMessage(pl_error_code(),
                              "pl_get_param(PARAM_FRAME_CAPABLE) error");
            return false;
        }
        if (m_isFrameTransfer == TRUE)
            qInfo("Camera with Frame Transfer capability sensor");
    }
    if (m_isFrameTransfer == FALSE) {
        m_isFrameTransfer = 0;
        qInfo("Camera without Frame Transfer capability sensor");
    }

    // If this is a Frame Transfer sensor set PARAM_PMODE to PMODE_FT.
    // The other common mode for these sensors is PMODE_ALT_FT.
    if (!isParamAvailable(PARAM_PMODE, "PARAM_PMODE"))
        return false;
    if (m_isFrameTransfer == TRUE) {
        int32 PMode = PMODE_FT;
        if (PV_OK != pl_set_param(m_hCam, PARAM_PMODE, reinterpret_cast<void*>(&PMode))) {
            printErrorMessage(pl_error_code(), "pl_set_param(PARAM_PMODE) error");
            return false;
        }
    // If not a Frame Transfer sensor (i.e. Interline), set PARAM_PMODE to
    // PMODE_NORMAL, or PMODE_ALT_NORMAL.
    } else {
        int32 PMode = PMODE_NORMAL;
        if (PV_OK != pl_set_param(m_hCam, PARAM_PMODE, reinterpret_cast<void*>(&PMode)))
        {
            printErrorMessage(pl_error_code(), "pl_set_param(PARAM_PMODE) error");
            return false;
        }
    }

    // This code iterates through all available camera ports and their readout
    // speeds and creates a Speed Table which holds indices of ports and speeds,
    // readout frequencies and bit depths.

    NVPC ports;
    if (!readEnumeration(&ports, PARAM_READOUT_PORT, "PARAM_READOUT_PORT"))
        return false;

    if (!isParamAvailable(PARAM_SPDTAB_INDEX, "PARAM_SPDTAB_INDEX"))
        return false;
    if (!isParamAvailable(PARAM_PIX_TIME, "PARAM_PIX_TIME"))
        return false;
    if (!isParamAvailable(PARAM_BIT_DEPTH, "PARAM_BIT_DEPTH"))
        return false;

    // Iterate through available ports and their speeds
    for (size_t pi = 0; pi < ports.size(); pi++) {
        // Set readout port
        if (PV_OK != pl_set_param(m_hCam, PARAM_READOUT_PORT,
                reinterpret_cast<void*>(&ports[pi].value))) {
            printErrorMessage(pl_error_code(),
                    "pl_set_param(PARAM_READOUT_PORT) error");
            return false;
        }

        // Get number of available speeds for this port
        uns32 speedCount;
        if (PV_OK != pl_get_param(m_hCam, PARAM_SPDTAB_INDEX, ATTR_COUNT,
                reinterpret_cast<void*>(&speedCount))) {
            printErrorMessage(pl_error_code(),
                    "pl_get_param(PARAM_SPDTAB_INDEX) error");
            return false;
        }

        // Iterate through all the speeds
        for (int16 si = 0; si < static_cast<int16>(speedCount); si++) {
            // Set camera to new speed index
            if (PV_OK != pl_set_param(m_hCam, PARAM_SPDTAB_INDEX, reinterpret_cast<void*>(&si))) {
                printErrorMessage(pl_error_code(),
                        "pl_set_param(m_hCam, PARAM_SPDTAB_INDEX) error");
                return false;
            }

            // Get pixel time (readout time of one pixel in nanoseconds) for the
            // current port/speed pair. This can be used to calculate readout
            // frequency of the port/speed pair.
            uns16 pixTime;
            if (PV_OK != pl_get_param(m_hCam, PARAM_PIX_TIME, ATTR_CURRENT,
                    reinterpret_cast<void*>(&pixTime))) {
                printErrorMessage(pl_error_code(),
                        "pl_get_param(m_hCam, PARAM_PIX_TIME) error");
                return false;
            }

            // Get bit depth of the current readout port/speed pair
            int16 bitDepth;
            if (PV_OK != pl_get_param(m_hCam, PARAM_BIT_DEPTH, ATTR_CURRENT,
                    reinterpret_cast<void*>(&bitDepth))) {
                printErrorMessage(pl_error_code(),
                        "pl_get_param(PARAM_BIT_DEPTH) error");
                return false;
            }

            int16 gainMin;
            if (PV_OK != pl_get_param(m_hCam, PARAM_GAIN_INDEX, ATTR_MIN,
                    reinterpret_cast<void*>(&gainMin))) {
                printErrorMessage(pl_error_code(),
                        "pl_get_param(PARAM_GAIN_INDEX) error");
                return false;
            }

            int16 gainMax;
            if (PV_OK != pl_get_param(m_hCam, PARAM_GAIN_INDEX, ATTR_MAX,
                    reinterpret_cast<void*>(&gainMax))) {
                printErrorMessage(pl_error_code(),
                        "pl_get_param(PARAM_GAIN_INDEX) error");
                return false;
            }

            int16 gainIncrement;
            if (PV_OK != pl_get_param(m_hCam, PARAM_GAIN_INDEX, ATTR_INCREMENT,
                    reinterpret_cast<void*>(&gainIncrement))) {
                printErrorMessage(pl_error_code(),
                        "pl_get_param(PARAM_GAIN_INDEX) error");
                return false;
            }

            // Save the port/speed information to our Speed Table
            READOUT_OPTION ro;
            ro.port = ports[pi];
            ro.speedIndex = si;
            ro.readoutFrequency = 1000 / static_cast<float>(pixTime);
            ro.bitDepth = bitDepth;
            ro.gains.clear();

            int16 gainValue = gainMin;

            while (gainValue <= gainMax) {
                ro.gains.push_back(gainValue);
                gainValue += gainIncrement;
            }

            m_SpeedTable.push_back(ro);

            qInfo("SpeedTable[%lu].Port = %ld (%d - %s)",
                  static_cast<unsigned long>(m_SpeedTable.size() - 1),
                  static_cast<unsigned long>(pi),
                  ro.port.value,
                  ro.port.name.c_str());
            qInfo("SpeedTable[%lu].SpeedIndex = %d",
                  static_cast<unsigned long>(m_SpeedTable.size() - 1),
                  ro.speedIndex);
            qInfo("SpeedTable[%lu].PortReadoutFrequency = %.3f MHz",
                  static_cast<unsigned long>(m_SpeedTable.size() - 1),
                  static_cast<double>(ro.readoutFrequency));
            qInfo("SpeedTable[%lu].bitDepth = %d bit",
                  static_cast<unsigned long>(m_SpeedTable.size() - 1),
                  ro.bitDepth);
            for (int16 gi = 0; gi < static_cast<int16>(ro.gains.size()); gi++) {
                qInfo("SpeedTable[%lu].gains[%d] = %d ",
                      static_cast<unsigned long>(m_SpeedTable.size() - 1),
                      static_cast<int>(gi),
                      ro.gains[static_cast<unsigned int>(gi)]);
            }
        }
    }

    if (!isParamAvailable(PARAM_SER_SIZE, "PARAM_SER_SIZE"))
        return false;
    if (PV_OK != pl_get_param(m_hCam, PARAM_SER_SIZE, ATTR_CURRENT,
            reinterpret_cast<void*>(&m_SensorResX)))
    {
        printErrorMessage(pl_error_code(), "Couldn't read CCD X-resolution");
        return false;
    }
    // Get number of sensor lines
    if (!isParamAvailable(PARAM_PAR_SIZE, "PARAM_PAR_SIZE"))
        return false;
    if (PV_OK != pl_get_param(m_hCam, PARAM_PAR_SIZE, ATTR_CURRENT,
            reinterpret_cast<void*>(&m_SensorResY))) {
        printErrorMessage(pl_error_code(), "Couldn't read CCD Y-resolution");
        return false;
    }
    qInfo("Sensor size: %dx%d", m_SensorResX, m_SensorResY);

    // Set number of sensor clear cycles to 2 (default)
    if (!isParamAvailable(PARAM_CLEAR_CYCLES, "PARAM_CLEAR_CYCLES"))
        return false;
    uns16 ClearCycles = 2;
    if (PV_OK != pl_set_param(m_hCam, PARAM_CLEAR_CYCLES, reinterpret_cast<void*>(&ClearCycles)))
    {
        printErrorMessage(pl_error_code(),
                "pl_set_param(PARAM_CLEAR_CYCLES) error");
        return false;
    }

    if (!isParamAvailable(PARAM_CLEAR_MODE, "PARAM_CLEAR_MODE"))
        return false;
    int32 ClearMode = CLEAR_PRE_EXPOSURE;
    if (PV_OK != pl_set_param(m_hCam, PARAM_CLEAR_MODE, reinterpret_cast<void*>(&ClearMode)))
    {
        printErrorMessage(pl_error_code(),
                "pl_set_param(PARAM_CLEAR_MODE) error");
        return false;
    }

    if (!isParamAvailable(PARAM_TEMP, "PARAM_TEMP")) {
        return false;
    }

    if (isParamAvailable(PARAM_TEMP_SETPOINT, "PARAM_TEMP_SETPOINT")) {
        if (PV_OK != pl_get_param(m_hCam, PARAM_TEMP_SETPOINT, ATTR_MIN,
                                  reinterpret_cast<void*>(&m_minSetTemperature)))
        {
            printErrorMessage(pl_error_code(),
                    "pl_get_param(PARAM_TEMP_SETPOINT, ATTR_MIN) error");
            return false;
        }

        // Update maximun setpoint value
        if (PV_OK != pl_get_param(m_hCam, PARAM_TEMP_SETPOINT, ATTR_MAX,
                reinterpret_cast<void*>(&m_maxSetTemperature)))
        {
            printErrorMessage(pl_error_code(),
                    "pl_get_param(PARAM_TEMP_SETPOINT, ATTR_MAX) error");
            return false;
        }
    }

    // Check Smart Streaming support on the camera.
    // We do not exit application here if the parameter is unavailable as this
    // parameter is available on Evolve-512 and Evolve-512 Delta only at the
    // moment.
    // We do not use isParamAvailable function as it print error messages
    // unwanted here.
    if (PV_OK != pl_get_param(m_hCam, PARAM_SMART_STREAM_MODE, ATTR_AVAIL,
            reinterpret_cast<void*>(&m_IsSmartStreaming))) {
        printErrorMessage(pl_error_code(),
                "Smart streaming availability check failed");
        return false;
    }
    if (m_IsSmartStreaming == TRUE)
        qInfo("Smart Streaming is available");
    else
        qInfo("Smart Streaming not available");

    setDefectivePixelCorrection(true);

    if (PV_OK != pl_cam_register_callback_ex3(m_hCam, PL_CALLBACK_EOF,
                                              reinterpret_cast<void*>(NewFrameHandler), this)) {
        printErrorMessage(pl_error_code(), "pl_cam_register_callback() error");
        return false;
    }
    qInfo("PL_CALLBACK_EOF callback registered successfully");

    return true;
}

bool WzPvCamera::closeCamera() {
    // Do not close camera if none has been detected and open
    if (m_isCameraOpen == TRUE)
    {
        if (PV_OK != pl_cam_close(m_hCam))
            printErrorMessage(pl_error_code(), "pl_cam_close() error");
        else
            qInfo("Camera closed");
    }

    return true;
}

bool WzPvCamera::getLatestFrame(uns16 **frameAddress) {
    // TODO 2020-3-1 写：这个函数里会将m_EofMutex解锁，而本函数中和后续函数中仍然对数据有一些并发访问，可能存在问题
    // 正确的方法应该是将预览数据复制完成后才解锁，解锁后相机会传送新的一帧数据过来
    if (!waitEof(5000)) {
        qWarning("waiting for preview data timeout");
        return false;
    }

    // Get the address of the latest frame in the circular buffer
    if (PV_OK != pl_exp_get_latest_frame(m_hCam, reinterpret_cast<void **>(frameAddress))) {
        printErrorMessage(pl_error_code(),
                "pl_exp_get_latest_frame() error");
        return false;
    }

    return true;
}

bool WzPvCamera::setupPreview() {
    // 设置高速率
    float readoutFrequency = 0;
    size_t speedIndex = 0;
    for (size_t i = 0; i < m_SpeedTable.size(); i++) {
        if (m_SpeedTable[i].readoutFrequency > readoutFrequency) {
            readoutFrequency = m_SpeedTable[i].readoutFrequency;
            speedIndex = i;
        }
    }
    if (PV_OK != pl_set_param(m_hCam, PARAM_READOUT_PORT,
            reinterpret_cast<void*>(&m_SpeedTable[speedIndex].port.value))) {
        printErrorMessage(pl_error_code(), "Readout port could not be set");
        return false;
    }
    qInfo("Setting readout port to %s", m_SpeedTable[speedIndex].port.name.c_str());

    // Set camera speed
    if (PV_OK != pl_set_param(m_hCam, PARAM_SPDTAB_INDEX,
            reinterpret_cast<void*>(&m_SpeedTable[speedIndex].speedIndex))) {
        printErrorMessage(pl_error_code(), "Readout port could not be set");
        return false;
    }
    qInfo("Setting readout speed index to %d", m_SpeedTable[0].speedIndex);

    // Set gain index to max
    if (PV_OK != pl_set_param(m_hCam, PARAM_GAIN_INDEX,
            reinterpret_cast<void*>(&m_SpeedTable[speedIndex].gains.back()))) {
        printErrorMessage(pl_error_code(), "Gain index could not be set");
        return false;
    }
    qInfo("Setting gain index to %d", m_SpeedTable[0].gains.back());

    return true;
}

bool WzPvCamera::startPreview() {
    QVariant binning = 1;
    getParam("Binning", binning);
    if (binning < 0) binning = 1;
    if (binning > 4) binning = 4;
    m_previewBinning = binning.toInt();
    rgn_type region;
    region.s1 = 0;
    region.s2 = m_SensorResX - 1;
    region.sbin = static_cast<uns16>(binning.toInt());
    region.p1 = 0;
    region.p2 = m_SensorResY - 1;
    region.pbin = static_cast<uns16>(binning.toInt());

    m_imageSize->setWidth(m_SensorResX / binning.toInt());
    m_imageSize->setHeight(m_SensorResY / binning.toInt());
    if (nullptr != m_previewImage8bit) {
        delete[] m_previewImage8bit;
        m_previewImage8bit = nullptr;
    }
    m_previewImage8bit = new uchar[m_imageSize->width() * m_imageSize->height()];
    if (nullptr != m_previewImage16bit) {
        delete [] m_previewImage16bit;
        m_previewImage16bit = nullptr;
    }
    m_previewImage16bitLength = m_imageSize->width() * m_imageSize->height();
    m_previewImage16bit = new uint16_t[m_previewImage16bitLength];
    m_imageBytes = static_cast<uint>(m_imageSize->width() * m_imageSize->height());

    QVariant exposureMs = 1;
    getParam("ExposureMs", exposureMs);
    uns32 exposureBytes;
    const uns32 exposureTime = exposureMs.toUInt();

    const uns16 circBufferFrames = 10;

    const int16 bufferMode = CIRC_OVERWRITE;

    if (PV_OK != pl_exp_setup_cont(m_hCam, 1, &region, TIMED_MODE,
            exposureTime, &exposureBytes, bufferMode))
    {
        printErrorMessage(pl_error_code(), "pl_exp_setup_cont() error");
        return false;
    }
    qDebug("Binning: %d", binning.toInt());
    qInfo("Circular buffer acquisition setup successful");

    uns32 previewBufferSize = circBufferFrames * exposureBytes / sizeof(uns16);
    if (m_previewBuffer != nullptr && m_previewBufferSize != previewBufferSize) {
        delete []m_previewBuffer;
        m_previewBuffer = nullptr;
    }
    m_previewBufferSize = previewBufferSize;
    if (m_previewBuffer == nullptr) {
        m_previewBuffer = new(std::nothrow) uns16[m_previewBufferSize];
    }
    if (m_previewBuffer == nullptr) {
        qWarning("Unable to allocate memory");
        return false;
    }

    if (PV_OK != pl_exp_start_cont(m_hCam, m_previewBuffer, m_previewBufferSize)) {
        printErrorMessage(pl_error_code(), "pl_exp_start_seq() error");
        return false;
    }
    qInfo("Circular buffer acquisition start successful");

    return true;
}

bool WzPvCamera::stopPreview() {
    m_isPreviewUpdated = false;
    m_EofFlag = false;
    // Once we have acquired the number of frames needed the acquisition can be
    // stopped, no other call is needed to stop the acquisition.
    if (PV_OK != pl_exp_stop_cont(m_hCam, CCS_CLEAR)) {
        printErrorMessage(pl_error_code(), "pl_exp_stop_cont() error");
        return false;
    }

    qInfo("Circular buffer mode acquisition stopped successfully");

    return true;
}

void WzPvCamera::newFrame() {
    {
        QMutexLocker lock(&m_EofMutex);
        m_EofFlag = true;
    }
    m_EofCond.notify_one();
}

void WzPvCamera::printErrorMessage(int16 errorCode, const char *message) {
    char pvcamErrMsg[ERROR_MSG_LEN];
    pl_error_message(errorCode, pvcamErrMsg);
    qWarning("%s, Error code: %d, Error message: %s", message, errorCode, pvcamErrMsg);
    m_latestErrorMsg = QString("%1, Error code: %2,Error message: %3").
            arg(message).arg(errorCode).arg(pvcamErrMsg);
}

bool WzPvCamera::isParamAvailable(uns32 paramID, const char *paramName) {
    if (paramName == nullptr)
        return false;

    rs_bool isAvailable;
    if (PV_OK != pl_get_param(m_hCam, paramID, ATTR_AVAIL, reinterpret_cast<void*>(&isAvailable))) {
        qWarning("Error reading ATTR_AVAIL of %s", paramName);
        return false;
    }
    if (!isAvailable) {
        qWarning("Parameter %s is not available", paramName);
        return false;
    }

    return true;
}

bool WzPvCamera::readEnumeration(NVPC *nvpc, uns32 paramID, const char *paramName) {
    if (nvpc == nullptr && paramName == nullptr)
        return false;

    if (!isParamAvailable(paramID, paramName))
        return false;

    uns32 count;
    if (PV_OK != pl_get_param(m_hCam, paramID, ATTR_COUNT, reinterpret_cast<void*>(&count))) {
        const std::string msg =
            "pl_get_param(" + std::string(paramName) + ") error";
        printErrorMessage(pl_error_code(), msg.c_str());
        return false;
    }

    // Actually get the triggering/exposure names
    for (uns32 i = 0; i < count; ++i) {
        // Ask how long the string is
        uns32 strLength;
        if (PV_OK != pl_enum_str_length(m_hCam, paramID, i, &strLength)) {
            const std::string msg =
                "pl_enum_str_length(" + std::string(paramName) + ") error";
            printErrorMessage(pl_error_code(), msg.c_str());
            return false;
        }

        // Allocate the destination string
        char *name = new (std::nothrow) char[strLength];

        // Actually get the string and value
        int32 value;
        if (PV_OK != pl_get_enum_param(m_hCam, paramID, i, &value, name, strLength)) {
            const std::string msg =
                "pl_get_enum_param(" + std::string(paramName) + ") error";
            printErrorMessage(pl_error_code(), msg.c_str());
            delete [] name;
            return false;
        }

        NVP nvp;
        nvp.value = value;
        nvp.name = name;
        nvpc->push_back(nvp);

        delete [] name;
    }

    return !nvpc->empty();
}

bool WzPvCamera::setDefectivePixelCorrection(const bool &isEnabled)
{
    uns32 featIndex;
    uns32 funcIndex;
    char featName[PARAM_NAME_LEN] = {0};
    char funcName[PARAM_NAME_LEN] = {0};
    uns32 curValue;
    uns32 newValue;

    // Defective pixel correction has index = 0
    featIndex = 0;
    if (pl_set_param(m_hCam, PARAM_PP_INDEX, &featIndex) == PV_FAIL) {
        qInfo("Error while setting feature index.");
        return false;
    }

    // Get feature name - optional
    if (pl_get_param(m_hCam, PARAM_PP_FEAT_NAME, int16(ATTR_CURRENT),
                     featName) == PV_FAIL) {

        qInfo("Error while getting feature name.");
        return false;
    }

    qDebug("Feature name : %s", featName);

    // Defective pixel only has one function, set it
    funcIndex = 0;
    if (pl_set_param(m_hCam, PARAM_PP_PARAM_INDEX, &funcIndex) == PV_FAIL) {
        qInfo("Error while setting fuction index.");
        return false;
    }

    // Get function name - Optional
    if (pl_get_param(m_hCam, PARAM_PP_PARAM_NAME, int16(ATTR_CURRENT),
                     &funcName) == PV_FAIL) {
        qInfo("Error while getting function name.");
        return false;
    }

    qDebug("Function name : %s", funcName);

    // Set value of this function 0 or 1 to enable or disable defective pixel correction
    newValue = isEnabled ? 1 : 0; //Enable , 0 to disable

    // Set value
    if (pl_set_param(m_hCam, PARAM_PP_PARAM, &newValue) == PV_FAIL) {
        qInfo("Error while setting function value.");
        return false;
    }

    // read back value
    if (pl_get_param(m_hCam, PARAM_PP_PARAM, int16(ATTR_CURRENT),
                     &curValue) == PV_FAIL) {
        qInfo("Error while getting function value.");
        return false;
    }

    if (curValue == newValue)
        qDebug("function value set to %d.", curValue);
    else
        qDebug("Failed to set value , Set Value = %d, Read Back = %d.",
               newValue,curValue);

    return newValue;
}

bool WzPvCamera::singleCaptureSetup() {
    QVariant binning = 0;
    getParam("Binning", binning);
    if (binning < 1)
        binning = 1;
    else if (binning > 4)
        binning = 4;

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
        m_grayAccumulateBuffer = new uns16[m_imageSize->width() * m_imageSize->height()];
        memset(m_grayAccumulateBuffer, 0, m_imageSize->width() * m_imageSize->height() * sizeof(uns16));
    }

    // 用最低速率和最高增益
    float minReadoutFrequency = m_SpeedTable[0].readoutFrequency;
    size_t speedIndex = 0;
    for (size_t i = 1; i < m_SpeedTable.size(); i++) {
        if (m_SpeedTable[i].readoutFrequency < minReadoutFrequency) {
            minReadoutFrequency = m_SpeedTable[i].readoutFrequency;
            speedIndex = i;
        }
    }
    if (PV_OK != pl_set_param(m_hCam, PARAM_READOUT_PORT,
            reinterpret_cast<void*>(&m_SpeedTable[speedIndex].port.value))) {
        printErrorMessage(pl_error_code(), "Readout port could not be set");
        return false;
    }
    qInfo("Setting readout port to %s, frequency: %g", m_SpeedTable[speedIndex].port.name.c_str(),
          static_cast<double>(m_SpeedTable[speedIndex].readoutFrequency));

    // Set camera speed
    if (PV_OK != pl_set_param(m_hCam, PARAM_SPDTAB_INDEX,
            reinterpret_cast<void*>(&m_SpeedTable[speedIndex].speedIndex))) {
        printErrorMessage(pl_error_code(), "Readout port could not be set");
        return false;
    }
    qInfo("Setting readout speed index to %d", m_SpeedTable[0].speedIndex);

    // Set gain index to max
    if (PV_OK != pl_set_param(m_hCam, PARAM_GAIN_INDEX,
            reinterpret_cast<void*>(&m_SpeedTable[speedIndex].gains.back()))) {
        printErrorMessage(pl_error_code(), "Gain index could not be set");
        return false;
    }
    qInfo("Setting gain index to %d", m_SpeedTable[0].gains.back());

    return true;
}

bool WzPvCamera::singleCaptureStart(uns32 exposureMs) {
    if (exposureMs < 1)
        exposureMs = 1;

    QVariant binning = 0;
    getParam("Binning", binning);
    if (binning < 1)
        binning = 1;
    else if (binning > 4)
        binning = 4;

    rgn_type region;
    region.s1 = 0;
    region.s2 = m_SensorResX - 1;
    region.sbin = static_cast<uns16>(binning.toInt());
    region.p1 = 0;
    region.p2 = m_SensorResY - 1;
    region.pbin = region.sbin;

    qInfo("Capture, exposure: %d, binning: %d, imageWidth: %d, imageHeight: %d",
          exposureMs, binning.toInt(), m_imageSize->width(), m_imageSize->height());

    uns32 imageBytes = 0;
    if (PV_OK != pl_exp_setup_seq(m_hCam, 1, 1, &region, TIMED_MODE,
            exposureMs, &imageBytes))
    {
        printErrorMessage(pl_error_code(), "pl_exp_setup_seq() error");
        return false;
    }
    qInfo("Acquisition setup successful");

    if (m_singleFrame != nullptr && m_imageBytes != imageBytes) {
        delete[] m_singleFrame;
        m_singleFrame = nullptr;
    }
    m_imageBytes = imageBytes;
    if (nullptr == m_singleFrame) {
        m_singleFrame = new (std::nothrow) uns16[m_imageBytes / sizeof(uns16)];
        if (m_singleFrame == nullptr) {
            qWarning("Unable to allocate memory");
            return false;
        }
    }

    if (PV_OK != pl_exp_start_seq(m_hCam, m_singleFrame)) {
        printErrorMessage(pl_error_code(), "pl_exp_start_seq() error");
        return false;
    }
    qInfo("Acquisition start successful");

    return true;
}

bool WzPvCamera::singleCaptureAbort() {
    if (PV_OK != pl_exp_abort(m_hCam, CCS_HALT)) {
        printErrorMessage(pl_error_code(), "pl_exp_abort(CCS_NO_CHANGE) error");
        return false;
    }
    qInfo("Abort the single capture");
    return true;
}

bool WzPvCamera::waitEof(uint milliseconds) {
    QMutexLocker lock(&m_EofMutex);
    if (!m_EofFlag)
        m_EofCond.wait(&m_EofMutex, milliseconds);
    if (!m_EofFlag) {
        return false;
    }    
    m_EofFlag = false;
    return true;
}

bool WzPvCamera::singleCaptureFinished() {
    if (PV_OK != pl_exp_finish_seq(m_hCam, m_singleFrame, 0)) {
        printErrorMessage(pl_error_code(), "pl_exp_finish_seq() error");
        return false;
    }
    return true;
}

double WzPvCamera::getTemperature() {
    return m_temperature;
}

int WzPvCamera::getMarkerImage(uint16_t **buffer)
{
    if (nullptr == m_previewImage16bit) {
        qWarning() << "getMarkerImage, error 1";
        return 0;
    }
    if (0 == m_previewImage16bitLength) {
        qWarning() << "getMarkerImage, error 2";
        return 0;
    }
    if (*buffer != nullptr) {
        delete [] (*buffer);
        *buffer = nullptr;
    }
    *buffer = new uint16_t[m_previewImage16bitLength];
    int countOfBytes = m_previewImage16bitLength * sizeof(m_previewImage16bit[0]);
    memcpy(*buffer, m_previewImage16bit, countOfBytes);
    return countOfBytes;
}

void WzPvCamera::updateTemperature() {
    //qDebug("updateTemperature");

    int16 temperature = 0;
    if (PV_OK != pl_get_param(m_hCam, PARAM_TEMP, ATTR_CURRENT, reinterpret_cast<void*>(&temperature))) {
        printErrorMessage(pl_error_code(), "pl_get_param(PARAM_TEMP) error");
        return;
    }
    m_temperature = temperature / 100;
    if (m_temperature < 0) {
        m_temperature = m_temperature * 1.5;
    }
    if (m_temperature < -28.5)
        m_temperature = -30;
}

bool WzPvCamera::setTemperature(double temperature) {
    if (temperature < 0)
        temperature = temperature / 1.5;
    int16 setpoint = static_cast<int16>(temperature * 100);
    if (PV_OK != pl_set_param(m_hCam, PARAM_TEMP_SETPOINT,
            reinterpret_cast<void*>(&setpoint)))
    {
        printErrorMessage(pl_error_code(),
                "pl_set_param(PARAM_TEMP_SETPOINT) error");
        return false;
    }
    return true;
}

bool getMaxAndAvgGray(uint16_t* grayBuffer, int countOfPixel,
    uint16_t* grayAvg, uint16_t* maxGray, int64_t* allGray)
{
    uint16_t curGray = 0;
    uint16_t* grayBufferPtr = grayBuffer;

    *maxGray = 0;
    *allGray = 0;
    grayBufferPtr = grayBuffer;

    for (int i = 0; i < countOfPixel; i++) {
        curGray = *grayBufferPtr;
        if (curGray > *maxGray)
            *maxGray = curGray;
        *allGray = *allGray + curGray;
        grayBufferPtr++;
    }

    *grayAvg = *allGray / countOfPixel;
    return true;
}

int WzPvCamera::autoExposure(int step, int destBin) {
    int bin = 4;
    qreal exposureMs = 1;

    if (step == 1) {
        bin = 2;
        exposureMs = 100;
    } else {
        bin = 4;
        exposureMs = 5000;
    }

    rgn_type region;
    region.s1 = 0;
    region.s2 = m_SensorResX - 1;
    region.sbin = bin;
    region.p1 = 0;
    region.p2 = m_SensorResY - 1;
    region.pbin = bin;

    int preGray = 12000;
    int imageWidth = m_SensorResX / region.sbin;
    int imageHeight = m_SensorResY / region.pbin;

    uns32 imageBytes = 0;
    if (PV_OK != pl_exp_setup_seq(m_hCam, 1, 1, &region, TIMED_MODE,
            exposureMs, &imageBytes))
    {
        printErrorMessage(pl_error_code(), "pl_exp_setup_seq() error");
        return false;
    }
    qInfo("pl_exp_setup_seq successful");

    uns16* imageBuffer = new (std::nothrow) uns16[imageBytes / sizeof(uns16)];
    if (imageBuffer == nullptr) {
        qWarning("Unable to allocate memory");
        return -1;
    }

    if (PV_OK != pl_exp_start_seq(m_hCam, imageBuffer)) {
        printErrorMessage(pl_error_code(), "pl_exp_start_seq() error");
        return -1;
    }
    qInfo("pl_exp_start_seq successful");

    // 总共等待20秒, 在小间隔时间内监测是不是取消了拍摄
    bool isTimeout = true;
    for (int n = 0; n < 40; n++) {
        if (m_isAbortCapture) {
            break;
        }
        if (waitEof(500)) {
            isTimeout = false;
            break;
        } else {
            qInfo("Waiting for the Eof flag, %d", n * 500);
        }
    }
    if (PV_OK != pl_exp_finish_seq(m_hCam, imageBuffer, 0)) {
        printErrorMessage(pl_error_code(), "pl_exp_finish_seq() error");
        return -1;
    }
    if (isTimeout) {
        qInfo("Auto exposure readout timeout");
    }

    WzImageFilter filter;
    filter.medianFilter(imageBuffer, imageWidth, imageHeight, 500);

    uint16_t grayAvg;
    uint16_t maxGray;
    int64_t allGray;
    getMaxAndAvgGray(imageBuffer, imageWidth * imageHeight, &grayAvg, &maxGray, &allGray);

    qDebug() << "Auto exposure, maxGray: " << maxGray << ", grayAvg: " << grayAvg;
    int caseNumber = 0;

    if (step == 1) {
        if (maxGray - grayAvg > 800) {
            exposureMs = static_cast<qreal>(preGray * 16) / static_cast<qreal>((1 * (maxGray - grayAvg) * destBin * destBin));
            caseNumber = 2;
        } else {
            exposureMs = autoExposure(2, destBin);
            caseNumber = 3;
        }
    } else {
        exposureMs = static_cast<qreal>(preGray * 16 * 5) / static_cast<qreal>((maxGray - grayAvg) * destBin * destBin);
        caseNumber = 4;
    }

    qDebug() << "AE-: " << caseNumber;

    if (exposureMs == 0)
        exposureMs = 1;

    delete [] imageBuffer;

    return exposureMs * 40;
}
