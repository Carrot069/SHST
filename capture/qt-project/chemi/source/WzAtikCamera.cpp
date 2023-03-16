#include "WzAtikCamera.h"

// TODO 监测相机断开事件

WzAtikCamera::WzAtikCamera(QObject *parent) :
    WzAbstractCamera(parent),
    m_isConnected(false),
    m_run(false),
    m_isPreviewEnabled(false),
    m_isPreviewUpdated(false),
    m_isCapture(false),
    m_imageSize(new QSize(672, 550))
{
    qDebug() << "WzAtikCamera";

    m_cameraParameters["ExposureMs"] = 10;
    m_cameraParameters["Binning"   ] = 4;

    m_timer = new QTimer(this);
    m_thread = new WzCameraThread(this);

    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(handleTimerFired()));
    m_timer->setSingleShot(false);
    m_timer->setInterval(10);
    m_timer->start();

    // 白光预览的灰阶显示范围 0 - 20000, 这样可以降低曝光时间, 加快单帧曝光时间
    m_gray16to8Table = new uint8_t[65536];
    for(uint i = 0; i < 65536; i++)
        m_gray16to8Table[i] = static_cast<uchar>(qMin<uint>(i / 80, 255));
}

WzAtikCamera::~WzAtikCamera()
{
    //disconnect();
    m_run = false;
    m_thread->wait(40000);

    delete m_imageSize;
    delete m_thread;
    delete m_timer;

    delete [] m_gray16to8Table;

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

void WzAtikCamera::connect()
{
    newCameraState(WzCameraState::Connecting);
    m_thread->start();
}

void WzAtikCamera::disconnect()
{
    m_run = false;
    m_thread->wait(15000);
    return;
}

void WzAtikCamera::run()
{
    qInfo() << "AtikCamera::run";

    bool isStartedCapture = false;
    bool isStartedExposure = false;
    bool isStartedPreview = false;
    bool isAutoExposureFinsied = false;
    QThread::msleep(1000);
    m_isConnected = false;
    m_run = true;
    newCameraState(WzCameraState::Connecting);

    QRandomGenerator rg;

    if (nullptr == m_cameraCallback) {
        qWarning("nullptr == m_cameraCallback");
    } else {
        if (initSDK()) {
            if (openCamera()) {
                if (m_cameraCallback->cameraSN("A" + QString(m_camSerNum))) {
                    setTemperature(-30);
                    m_isConnected = true;
                    newCameraState(WzCameraState::Connected);
                } else {
                    newCameraState(WzCameraState::Disconnected);
                    closeCamera();
                    uninitSdk();
                }
            } else {
                newCameraState(WzCameraState::Disconnected);
            }
        } else {
            newCameraState(WzCameraState::CameraNotFound);
            uninitSdk();
        }
    }

    QVariant previewExposureMs = 1;
    int pixels = 0;

    while (m_run) {
        if (!m_isCameraOpen) {
            QThread::msleep(10);
            continue;
        }
        if (m_isPreviewEnabled) {
            if (!isStartedPreview) {
                m_resetPreview = false;
                if (setupPreview() && startPreview()) {
                    getParam("ExposureMs", previewExposureMs);
                    isStartedPreview = true;
                    newCameraState(WzCameraState::PreviewStarted);
                } else {
                    m_isPreviewEnabled = false;
                    newCameraState(WzCameraState::PreviewStopped);
                    continue;
                }
            } else {
                bool gotImageData = false;
                for (int i = 0; i < 5; i++) {
                    if (m_resetPreview || !m_isPreviewEnabled)
                        break;
                    gotImageData = getLatestFrame(m_previewBuffer, 1000);
                    if (gotImageData) {
                        pixels = m_imageSize->width() * m_imageSize->height();
                        m_imageBytes = static_cast<uint>(m_imageSize->width() * m_imageSize->height());
                        m_previewImage16bitLength = m_imageSize->width() * m_imageSize->height();
                        m_previewBufferSize = m_imageSize->width() * m_imageSize->height();
                        break;
                    } else {
                        if (CAMERA_IDLE == ArtemisCameraState(handle)) {
                            break;
                        }
                    }
                }
                if (gotImageData && m_isPreviewEnabled && !m_resetPreview) {
                    QMutexLocker lock(&m_mutexImageData);

                    WzUtils::calc16bitTo8bit(m_previewBuffer,
                                             static_cast<uint32_t>(pixels),
                                             m_gray16to8Table,
                                             4000);
                    for (int i = 0; i < pixels; i++) {
                        m_previewImage8bit[i] = m_gray16to8Table[m_previewBuffer[i]];
                    }

                    //std::ofstream outFile("D:/image.raw", std::ios::out | std::ios::binary);
                    //outFile.write((char*)m_previewImage8bit, m_imageSize->width() * m_imageSize->height());
                    //outFile.close();

                    {
                        QMutexLocker lock2(&m_previewImage16bitMutex);
                        memcpy_s(m_previewImage16bit, m_previewImage16bitLength * sizeof(m_previewImage16bit[0]),
                                 m_previewBuffer, m_previewImage16bitLength * sizeof(m_previewImage16bit[0]));
                    }

                    m_pImageData = m_previewImage8bit;
                    m_isPreviewUpdated = true;
                }
                if (m_isPreviewEnabled && !m_resetPreview) {
                    if (m_needResetBin) {
                        m_needResetBin = false;
                        qInfo() << "needResetBin == true, stop, setup, start preview";
                        stopPreview();
                        setupPreview();
                        startPreview();
                    }
                    else if (CAMERA_IDLE == ArtemisCameraState(handle)) {
                        continuePreview(previewExposureMs.toInt());
                    }
                }

                if (m_resetPreview) {
                    m_resetPreview = false;
                    // 2021-09-29 不停止预览就可以改变曝光时间, 目前调用 resetPreview 只是为了修改曝光时间
                    // 所以此款相机调用 resetPreview 后不需要停止和重新开始预览
                    getParam("ExposureMs", previewExposureMs);
                    /*
                    isStartedPreview = false;
                    stopPreview();
                    while(CAMERA_IDLE != ArtemisCameraState(handle)) {
                        qDebug("Wait stopping preivew");
                    }
                    startPreview();
                    */
                }
            }
        } else {
            if (isStartedPreview) {
                isStartedPreview = false;
                if (stopPreview()) {
                    m_cameraState = WzCameraState::PreviewStopped;
                    newCameraState(m_cameraState);
                } else {
                    // TODO 停止预览失败了
                }
            }
        }
        if (m_isCapture) {
            if (m_cameraState == WzCameraState::PreviewStarted) {
                m_isPreviewEnabled = false;
                isStartedPreview = false;
                newCameraState(WzCameraState::PreviewStopping);
                if (stopPreview()) {
                    newCameraState(WzCameraState::PreviewStopped);
                } else {
                    // TODO 处理停止失败
                }
            }
            // 拍摄前的初始化
            if (!isStartedCapture) {
                newCameraState(WzCameraState::CaptureInit);

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
                newCameraState(WzCameraState::AutoExposure);

                QVariant binning = 1;
                getParam("Binning", binning);
                if (binning < 0) binning = 1;
                if (binning > 4) binning = 4;

                isAutoExposureFinsied = true;
                int aeMs = autoExposure(1, binning.toInt());
                qInfo("AE ms: %d", aeMs);
                if (aeMs <= 0)
                    aeMs = 11111;
                m_exposureMilliseconds[0] = aeMs;
                m_leftExposureMilliseconds = m_exposureMilliseconds[0];
                newCameraState(WzCameraState::AutoExposureFinished);
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
                newCameraState(WzCameraState::CaptureAborted);
                continue;
            }
            if (isStartedCapture && !isStartedExposure) {
                newCameraState(WzCameraState::Exposure);
                // TODO 考虑启动曝光失败了如何处理
                if (singleCaptureStart(static_cast<uint32_t>(m_exposureMilliseconds[m_currentFrame]))) {
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
                    isAutoExposureFinsied = false;
                }

                newCameraState(WzCameraState::Image);

#ifdef EXPOSURE_TIME_DOUBLE
                // 把自动加上去的曝光时间也等完
                while (m_additionExposureMs > 0) {
                    if (m_isAbortCapture) {
                        break;
                    }
                    QThread::msleep(500);
                    m_additionExposureMs -= 500;
                }
#endif

                // 总共等待20秒, 在小间隔时间内监测是不是取消了拍摄
                bool isTimeout = true;
                for (int n = 0; n < 40; n++) {
                    if (m_isAbortCapture) {
                        break;
                    }
                    if (getLatestFrame(m_singleFrame, 500)) {
                        m_imageBytes = m_imageSize->width() * m_imageSize->height() * sizeof(uint16_t);
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
                    newCameraState(WzCameraState::CaptureAborted);
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
                newCameraState(WzCameraState::CaptureFinished);
                QThread::msleep(200); // 必须停一下, 防止上层逻辑检测不到这个状态就变掉了

                if (m_currentFrame+1 < m_frameCount) {
                    m_currentFrame++;
                    m_leftExposureMilliseconds = m_exposureMilliseconds[m_currentFrame];
                }
            }
        }


        QThread::msleep(10);
    }

    newCameraState(WzCameraState::Disconnecting);
    QThread::msleep(1000);
    newCameraState(WzCameraState::Disconnected);

    closeCamera();
    uninitSdk();
}

int WzAtikCamera::setPreviewEnabled(bool enabled)
{
    qDebug() << "setPreviewEnabled, enabled=" << enabled;
    if (enabled) {
        newCameraState(WzCameraState::PreviewStarting);
    } else {
        newCameraState(WzCameraState::PreviewStopping);
    }
    m_isPreviewEnabled = enabled;
    return ERROR_NONE;
}

void WzAtikCamera::resetPreview()
{
    qInfo() << "AtikCamera::resetPreview";
    m_resetPreview = true;
}

int WzAtikCamera::getImage(void* imageData)
{
    QMutexLocker lock(&m_mutexImageData);
    memcpy_s(imageData, m_imageBytes, m_pImageData, static_cast<size_t>(m_imageBytes));
    //std::ofstream outFile("D:/image.raw", std::ios::out | std::ios::binary);
    //outFile.write((char*)imageData, m_imageBytes);
    //outFile.close();
    return ERROR_NONE;
}

int WzAtikCamera::capture(const int exposureMilliseconds)
{
    int ms[] = {exposureMilliseconds};
    return capture(ms, 1);
}

int WzAtikCamera::capture(const int* exposureMilliseconds, const int count)
{
    if (count == 0) return ERROR_PARAM;
    m_frameCount = count;
    if (m_exposureMilliseconds)
        delete[] m_exposureMilliseconds;
    m_exposureMilliseconds = new int[count];
    memcpy(m_exposureMilliseconds, exposureMilliseconds, static_cast<uint>(count) * sizeof(int));
    m_isAbortCapture = false;
    m_isCapture = true;
    qDebug() << "WzACamera::capture";
    return ERROR_NONE;
}

void WzAtikCamera::abortCapture() {
    qDebug() << "WzACamera::abortCapture(), m_isAbortCapture:" << m_isAbortCapture;
    if (m_isAbortCapture)
        return;
    m_isAbortCapture = true;
    newCameraState(WzCameraState::CaptureAborting);
    qDebug() << "WzACamera::abortCapture()";
}

void WzAtikCamera::handleTimerFired()
{
    if (m_isPreviewEnabled && m_isPreviewUpdated) {
        emitPreviewImageUpdated();
        m_isPreviewUpdated = false;
    } else if (m_isCaptureFinished) {
        emitCaptureFinished();
        m_isCaptureFinished = false;
    }
}

QSize* WzAtikCamera::getImageSize() {
    return m_imageSize;
}

int WzAtikCamera::getExposureMs() {
    if (nullptr == m_exposureMilliseconds)
        return 1;
    return m_exposureMilliseconds[m_currentFrame];
}

int WzAtikCamera::getLeftExposureMs() {
    return m_leftExposureMilliseconds;
}

int WzAtikCamera::getCurrentFrame() {
    return m_currentFrame;
}

int WzAtikCamera::getCapturedCount() {
    return m_capturedCount;
}

int WzAtikCamera::getImageBytes() {
    return static_cast<int>(m_imageBytes);
}

void WzAtikCamera::processImage() {
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
            qFatal("m_grayAccumulateBuffer == nullptr");
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

    QVariant removeFluorCircle;
    getParam("removeFluorCircle", removeFluorCircle);
    if (removeFluorCircle.toBool()) {
        imageBuffer.update();
        WzImageService::removePlateBorder(imageBuffer);
    }

    QString path = mkdirImagePath();
    m_latestImageFile = getTiffFileName(path, imageBuffer.captureDateTime);
    WzImageService::saveImageAsTiff(imageBuffer, m_latestImageFile);

    imageBuffer.update();
    QVariant isThumbNegative = true;
    getParam("isThumbNegative", isThumbNegative);
    m_latestThumbFile = WzImageService::createThumb(imageBuffer, m_latestImageFile, isThumbNegative.toBool());

    // get min/max gray from marker
    {
        QMutexLocker lock(&m_previewImage16bitMutex);
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
}


QString WzAtikCamera::getLatestImageFile() {
    return m_latestImageFile;
};

QString WzAtikCamera::getLatestThumbFile() {
    return m_latestThumbFile;
}

bool WzAtikCamera::initSDK() {
    qInfo() << "AtikCamera::initSDK";
    // First: Try to load the DLL:
    QString dllFileName = qApp->applicationDirPath() + "/atik/AtikCameras.dll";
    QString fn = QDir::toNativeSeparators(dllFileName);
    wchar_t w[65535] = {0};
    fn.toWCharArray(w);
    if (!ArtemisLoadDLL(w))
    {
        qInfo("Cannot Load DLL");
        return false;
    }

    // Now Check API / DLL versions
    int apiVersion = ArtemisAPIVersion();
    int dllVersion = ArtemisDLLVersion();
    if (apiVersion != dllVersion)
    {
        qInfo("Version do not match! API: %d DLL: %d", apiVersion, dllVersion);
        return false;
    }

    qInfo("API Version: %d", apiVersion);
    qInfo("DLL Version: %d", dllVersion);

    m_isSdkInitialized = true;
    qInfo("A Sdk initialized");

    return true;
}

bool WzAtikCamera::uninitSdk() {
    qInfo() << "AtikCamera::uninitSdk";
    if (m_isSdkInitialized) {
        ArtemisShutdown();
        try {
            //ArtemisUnLoadDLL();
        } catch(std::exception& e) {
            qWarning("Unload dll exception: %s", e.what());
        }
        m_isSdkInitialized = false;
        qInfo("A Sdk uninitializated");
    }

    return true;
}

bool WzAtikCamera::openCamera() {
    m_numberOfCameras = ArtemisDeviceCount();
    if (m_numberOfCameras == 0) {
        qInfo("A Camera is 0");
        return false;
    }

    handle = ArtemisConnect(0);
    if (handle == NULL) {
        qInfo("No Camera Available");
        return false;
    }

    m_isCameraOpen = true;
    qInfo("Camera opened");

    // Read the serial number of Camera
    if (!ArtemisDeviceSerial(0, m_camSerNum)) {
        return false;
    }
    qInfo("Camera number: %s", m_camSerNum);

    ARTEMISPROPERTIES properties;
    if (ARTEMIS_OK != ArtemisProperties(handle, &properties)) {
        qInfo("Can't get properties");
        return false;
    }

    m_nTempSensor = 0;
    if (ARTEMIS_OK != ArtemisTemperatureSensorInfo(handle, 0, &m_nTempSensor)) {
        qInfo("Can't get sensor info");
        return false;
    }
    if (m_nTempSensor == 0)
        setParam("AtikTemperatureSensorZero", true);

    qInfo("Name:           \t%s",		   properties.Description);
    qInfo("Manufacturer:   \t%s",		   properties.Manufacturer);
    qInfo("Pixels:         \t%d x %d",      properties.nPixelsX,      properties.nPixelsY);
    qInfo("Pixel Size:     \t%.2f x %.2f",  properties.PixelMicronsX, properties.PixelMicronsY);
    qInfo("Has Cooling:    \t%s",		   (m_nTempSensor > 0) ? "True" : "False");

    m_SensorResX = properties.nPixelsX;
    m_SensorResY = properties.nPixelsY;

    return true;
}

bool WzAtikCamera::closeCamera() {
    qInfo() << "AtikCamera::closeCamera";
    if (m_isCameraOpen) {
        if (!ArtemisDisconnect(handle)) {
            qInfo("Can't close camera");
            return false;
        } else {
            qInfo("Camera closed");
            m_isCameraOpen = false;
        }
    }

    return true;
}

bool WzAtikCamera::getLatestFrame(uint16_t *frameAddress, const int timeout) {
    qInfo() << "AtikCamera::getLatestFrame";

    QElapsedTimer et;
    et.start();

    int cameraState = 0;
    while (!ArtemisImageReady(handle))
    {
        int newState = ArtemisCameraState(handle);
        //qInfo("Camera state: %d", newState);

        if (newState != cameraState)
        {
            cameraState = newState;

            switch (cameraState)
            {
            case CAMERA_ERROR:
                qWarning("Something went wrong!!");
                return false;
                break;

            case CAMERA_IDLE:
                qDebug("Idle");
                break;

            case CAMERA_WAITING:
                qDebug("Waiting");
                break;

            case CAMERA_EXPOSING:
                qDebug("Exposing");
                if (m_isAbortGetImageData) {
                    return false;
                }
                break;

            case CAMERA_READING:
                qDebug("Reading");
                break;

            case CAMERA_DOWNLOADING:
                qDebug("Downloading");
                break;

            case CAMERA_FLUSHING:
                qDebug("Flushing");
                break;
            }
        }

        if (cameraState == CAMERA_DOWNLOADING)
        {
            // int percent = ArtemisDownloadPercent(handle);
            // qDebug("Download: %d %%", percent);
        }

        if (et.elapsed() >= timeout) {
            qWarning("Get image data timeout(%d)", timeout);
            qWarning("state:(%d)", newState);
            int binX, binY;
            int error = ArtemisGetBin(handle, &binX, &binY);
            if (error != ARTEMIS_OK) {
                qWarning("GetBin error: %d", error);
            } else {
                if (binX != binY) {
                    qInfo() << "binX != binY, need reset bin";
                    this->m_needResetBin = true;
                }
            }
            return false;
        }

        QThread::msleep(1);
    }

    if (m_isAbortGetImageData) {
        return false;
    }

    qInfo("Image Finished");

    int x, y, w, h, xBin, yBin;
    int error = ArtemisGetImageData(handle, &x, &y, &w, &h, &xBin, &yBin);
    if (error != ARTEMIS_OK)
    {
        qWarning("Get Image Data: (%d)", error);
        return false;
    }
    else
    {
        m_imageSize->setHeight(h);
        m_imageSize->setWidth(w);
        qInfo("Width: %d, Height: %d, BinX: %d, BinY: %d", w, h, xBin, yBin);
    }

    void * imageBuffer = ArtemisImageBuffer(handle);
    if (imageBuffer == NULL)
    {
        qWarning("Get Image Buffer: %d", error);
        return false;
    }

    // 旋转180度
    int pixels = w * h;
    uint16_t* pSrc = static_cast<uint16_t*>(imageBuffer);
    for (int i = 0; i < pixels / 2; i++) {
        frameAddress[i] = pSrc[pixels - 1 - i];
        frameAddress[pixels - 1 - i] = pSrc[i];
    }

    return true;
}

bool WzAtikCamera::setupPreview() {
    qInfo() << "AtikCamera::setupPreview";
    QVariant binning;
    getParam("Binning", binning);
    int error = ARTEMIS_OK;
    if (binning.toInt() < 3)
        error = ArtemisSetPreviewEx(handle, true, ARTEMIS_PREVIEW_QUALITY_AVERAGE);
    else
        error = ArtemisSetPreviewEx(handle, true, ARTEMIS_PREVIEW_QUALITY_FULL_QUALITY);
    if (error != ARTEMIS_OK) {
        qWarning("set preview error: %d", error);
        return false;
    }

    // 按照最大图片尺寸申请缓冲区, 避免多次释放和申请内存
    if (!m_previewImage8bit) {
        m_previewImage8bit = new(std::nothrow) uint8_t[m_SensorResX * m_SensorResY];
        if (!m_previewImage8bit) {
            qWarning("Unable to allocate memory");
            return false;
        }
    }

    {
        QMutexLocker lock(&m_previewImage16bitMutex);
        if (!m_previewImage16bit) {
            m_previewImage16bit = new(std::nothrow) uint16_t[m_SensorResX * m_SensorResY];
            if (!m_previewImage16bit) {
                qWarning("Unable to allocate memory");
                return false;
            }
        }
    }

    if (!m_previewBuffer) {
        m_previewBuffer = new(std::nothrow) uint16_t[m_SensorResX * m_SensorResY];
        if (m_previewBuffer == nullptr) {
            qWarning("Unable to allocate memory");
            return false;
        }
    }

    return true;
}

bool WzAtikCamera::startPreview() {
    qInfo("AtikCamera::startPreview");

    m_isAbortGetImageData = false;

    QVariant binning = 1;
    getParam("Binning", binning);
    if (binning < 0) binning = 1;
    if (binning > 4) binning = 4;
    if (m_previewBinning != binning.toInt())
        m_previewBinning = binning.toInt();
    int binX, binY;
    ArtemisGetBin(handle, &binX, &binY);
    if (binX != m_previewBinning || binY != m_previewBinning) {
        int error = ArtemisBin(handle, m_previewBinning, m_previewBinning);
        if (error == ARTEMIS_OK)
            qDebug("Set Binning: X=%d, Y=%d", m_previewBinning, m_previewBinning);
        else
            qWarning("Set Binning error: %d", error);
    }

    QVariant exposureMs = 1;
    getParam("ExposureMs", exposureMs);
    const uint32_t exposureTime = exposureMs.toUInt();

    int error = ArtemisStartExposureMS(handle, exposureTime);
    if (error != ARTEMIS_OK) {
        qWarning("Start exposure error: %d", error);
        return false;
    }
    qInfo("Start preview");

    return true;
}

bool WzAtikCamera::continuePreview(const int exposureTime)
{
    qDebug() << "AtikCamera::continuePreview";
    int error = ArtemisStartExposureMS(handle, exposureTime);
    if (error != ARTEMIS_OK) {
        qWarning("Continue exposure error: %d", error);
        return false;
    }
    qDebug("continue preview");
    return true;
}

bool WzAtikCamera::stopPreview() {
    qInfo() << "AtikCamera::stopPreview";
    m_isPreviewUpdated = false;
    m_isAbortGetImageData = true;
    int error = ArtemisStopExposure(handle);
    if (error != ARTEMIS_OK) {
        qWarning("Stop preview error: %d", error);
        return false;
    }

    qInfo("Stop preview");

    return true;
}

bool WzAtikCamera::singleCaptureSetup() {
    QVariant binning = 0;
    getParam("Binning", binning);
    if (binning < 1)
        binning = 1;
    else if (binning > 4)
        binning = 4;

    int error = ArtemisBin(handle, binning.toInt(), binning.toInt());
    if (ARTEMIS_OK != error) {
        qWarning("Set bin error: %d", error);
        return false;
    }

    error = ArtemisSetPreviewEx(handle, false, ARTEMIS_PREVIEW_QUALITY_FULL_QUALITY);
    if (ARTEMIS_OK != error) {
        qWarning("Set preview error: %d", error);
        return false;
    }

    if (!m_singleFrame) {
        m_singleFrame = new (std::nothrow) uint16_t[m_SensorResX * m_SensorResY];
        if (m_singleFrame == nullptr) {
            qWarning("Unable to allocate memory");
            return false;
        }
    }

    // 灰度累积缓冲区处理
    // 处理灰度累积
    QVariant isGrayAccumulate = false;
    if (getParam("grayAccumulate", isGrayAccumulate) && isGrayAccumulate == true) {
        if (!m_grayAccumulateBuffer) {
            m_grayAccumulateBuffer = new uint16_t[m_SensorResX * m_SensorResY];
        }
        memset(m_grayAccumulateBuffer, 0, m_imageSize->width() * m_imageSize->height() * sizeof(uint16_t));
    }   

    QVariant gain = 0;
    QVariant gainOffset = 0;
    getParam("Gain", gain);
    getParam("GainOffset", gainOffset);
    ArtemisSetGain(handle, false, gain.toInt(), gainOffset.toInt());

    return true;
}

bool WzAtikCamera::singleCaptureStart(uint32_t exposureMs) {
    if (exposureMs < 1)
        exposureMs = 1;

    QVariant binning = 0;
    getParam("Binning", binning);
    if (binning < 1)
        binning = 1;
    else if (binning > 4)
        binning = 4;

    m_isAbortGetImageData = false;

    qInfo("Capture, exposure: %d, binning: %d, imageWidth: %d, imageHeight: %d",
          exposureMs, binning.toInt(), m_imageSize->width(), m_imageSize->height());

#ifdef EXPOSURE_TIME_DOUBLE
    m_realExposureMs = exposureMs * m_exposureTimeMultiplier;
    m_additionExposureMs = m_realExposureMs - exposureMs;
    exposureMs = m_realExposureMs;
    qDebug() << "m_realExposureMs:" << m_realExposureMs
             << ", m_additionExposureMs:" << m_additionExposureMs
             << ", exposureMs:" << exposureMs;
#endif

    m_imageBytes = sizeof(m_singleFrame[0]) * m_imageSize->width() * m_imageSize->height();

    int error = ArtemisBin(handle, binning.toInt(), binning.toInt());
    if (error == ARTEMIS_OK)
        qDebug("Set Binning: X=%d, Y=%d", binning.toInt(), binning.toInt());
    else
        qWarning("Set Binning error: %d", error);

    error = ArtemisStartExposureMS(handle, exposureMs);
    if (ARTEMIS_OK != error) {
        qWarning("Start exposure error: %d", error);
        return false;
    }

    qInfo("Start exposure");

    return true;
}

bool WzAtikCamera::singleCaptureAbort() {
    qInfo() << "AtikCamera::singleCaptureAbort";
    qDebug() << "Camera state:" << ArtemisCameraState(handle);
    if (CAMERA_EXPOSING == ArtemisCameraState(handle)) {
        if (ARTEMIS_OK != ArtemisAbortExposure(handle)) {
            qWarning("Abort exposure error");
            return false;
        }
    }

    qInfo("Abort the single capture");
    return true;
}

bool WzAtikCamera::singleCaptureFinished() {
    return true;
}

double WzAtikCamera::getTemperature() {
    updateTemperature();
    return m_temperature;
}

int WzAtikCamera::getMarkerImage(uint16_t **buffer)
{
    QMutexLocker lock(&m_previewImage16bitMutex);
    if (nullptr == m_previewImage16bit) {
        qWarning() << "getMarkerImage, buffer nullptr";
        return ERROR_NULLPTR;
    }
    if (0 == m_previewImage16bitLength) {
        qWarning() << "getMarkerImage, buffer length 0";
        return ERROR_DATA_LEN_0;
    }
    setParam("imageWidth", m_imageSize->width());
    setParam("imageHeight", m_imageSize->height());
    if (*buffer != nullptr) {
        delete [] (*buffer);
        *buffer = nullptr;
    }
    *buffer = new uint16_t[m_previewImage16bitLength];
    int countOfBytes = m_previewImage16bitLength * sizeof(m_previewImage16bit[0]);
    memcpy(*buffer, m_previewImage16bit, countOfBytes);
    return ERROR_NONE;
}

void WzAtikCamera::updateTemperature() {
    //qDebug("updateTemperature");

    if (m_nTempSensor == 0) {
        //qWarning("No Cooling Available");
        return;
    }

    int temp = 0;
    for (int i = 0; i < m_nTempSensor; i++) {
        int error = ArtemisTemperatureSensorInfo(handle, i + 1, &temp);
        if (error == ARTEMIS_OK) {
            //qDebug("Sensor: %d   Temp: %.2f", i + 1, (0.01 * temp));
        } else {
            qWarning("Sensor: %d", error);
        }
    }

    //qDebug() << "[cameraT]" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << temperature;
    m_temperature = temp / 100;
    if (m_temperature < 0) {
        m_temperature = m_temperature * 2.3;
    }
    if (m_temperature < -30)
        m_temperature = -30;
}

bool WzAtikCamera::setTemperature(double temperature) {
    qInfo() << "AtikCamera::setTemperature";
    /*
    if (temperature < 0)
        temperature = temperature / 1.5;
    */
    int setpoint = static_cast<int16>(temperature * 100);

    int error = ArtemisSetCooling(handle, setpoint);
    if (error == ARTEMIS_OK)
        return true;
    else {
        qWarning("Cooling Set: %d", error);
        return false;
    }
}

int WzAtikCamera::autoExposure(int step, int destBin) {
    int bin = 4;
    qreal exposureMs = 1;

    if (step == 1) {
        bin = 2;
        exposureMs = 100;
    } else {
        bin = 4;
        exposureMs = 5000;
    }

    int preGray = 12000;

    int error = ArtemisBin(handle, bin, bin);
    if (error == ARTEMIS_OK)
        qDebug("Set Binning: X=%d, Y=%d", m_previewBinning, m_previewBinning);
    else
        qWarning("Set Binning error: %d", error);

    if (ARTEMIS_OK != ArtemisStartExposureMS(handle, exposureMs)) {
        qWarning("Start exposure error");
        return -1;
    }
    qInfo("Start exposure successful");

    m_isAbortGetImageData = false;
    while (ArtemisExposureTimeRemaining(handle)) {
        //qInfo("Wait exposure");
        if (m_isAbortCapture) {
            break;
        }
        QThread::msleep(10);
    }

    uint32_t imageBytes = m_SensorResX * m_SensorResY * sizeof(uint16_t);
    uint16_t* imageBuffer = new (std::nothrow) uint16_t[imageBytes / sizeof(uint16_t)];
    if (imageBuffer == nullptr) {
        qWarning("Unable to allocate memory");
        return -1;
    }

    // 总共等待20秒, 在小间隔时间内监测是不是取消了拍摄
    bool isTimeout = true;
    for (int n = 0; n < 20; n++) {
        if (getLatestFrame(imageBuffer, 1000)) {
            isTimeout = false;
            break;
        }
        if (m_isAbortCapture) {
            break;
        }
    }
    if (isTimeout) {
        qInfo("Auto exposure readout timeout");
        delete [] imageBuffer;
        return -1;
    }

    int imageWidth = m_imageSize->width();
    int imageHeight = m_imageSize->height();
    WzImageFilter filter;
    filter.filterLightspot(imageBuffer, imageWidth, imageHeight);
    qInfo("AE, fend");

    uint16_t grayAvg;
    uint16_t maxGray;
    int64_t allGray;
    getMaxAndAvgGray(imageBuffer, imageWidth * imageHeight, &grayAvg, &maxGray, &allGray);

    qInfo() << "AE, m: " << maxGray << ", a: " << grayAvg;
    int caseNumber = 0;

    if (maxGray == 0) {
        return -1;
    }
    if (grayAvg == maxGray) {
        return 1;
    }

    if (step == 1) {
        if (maxGray - grayAvg > 1000) {
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

    qInfo() << "AE-C: " << caseNumber;

    if (exposureMs <= 0)
        exposureMs = 1;

    delete [] imageBuffer;

    return exposureMs * 40;
}
