#include "WzCamera.h"

WzCameraThread::WzCameraThread(QObject *parent) : QThread(parent) {
    m_camera = reinterpret_cast<WzAbstractCamera*>(parent);
}

void WzCameraThread::run() {
    m_camera->run();
}

WzTestCamera::WzTestCamera(QObject *parent) :
    WzAbstractCamera(parent),
    m_isConnected(false),
    m_run(false),
    m_isPreviewEnabled(false),
    m_isPreviewUpdated(false),
    m_isCapture(false),
    m_imageSize(new QSize(DEMO_CAMERA_WIDTH, DEMO_CAMERA_HEIGHT))
{
    qDebug() << "WzTestCamera";

    m_cameraParameters["ExposureMs"] = 0;
    m_cameraParameters["Gain"      ] = 0;
    m_cameraParameters["Binning"   ] = 1;

    m_timer = new QTimer(this);
    m_thread = new WzCameraThread(this);

    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(handleTimerFired()));
    m_timer->setSingleShot(false);
    m_timer->setInterval(10);
    m_timer->start();

    for(int i=0; i<65536; i++)
        m_gray16to8Table[i] = static_cast<uchar>(i / 256);

    m_imageData = new uint16_t[DEMO_CAMERA_WIDTH * DEMO_CAMERA_HEIGHT];
    m_previewImageData = new uchar[DEMO_CAMERA_WIDTH * DEMO_CAMERA_HEIGHT];
}

WzTestCamera::~WzTestCamera()
{
    //disconnect();
    m_run = false;
    m_thread->wait(5000);

    delete m_imageSize;
    delete m_thread;
    delete m_timer;

    delete [] m_imageData;
    delete [] m_previewImageData;

    if (m_exposureMilliseconds)
        delete[] m_exposureMilliseconds;

    qDebug() << "~WzTestCamera";
}

void WzTestCamera::connect()
{
    m_cameraState = WzCameraState::Connecting;
    emit cameraState(m_cameraState);
    m_thread->start();
}

void WzTestCamera::disconnect()
{
    m_run = false;
    m_thread->wait(5000);
    return;
}

void WzTestCamera::run()
{
    bool isInitCapture = false;
    bool isStartedPreview = false;
    QThread::msleep(1000);
    m_isConnected = true;
    m_run = true;
    m_cameraState = WzCameraState::Connected;
    emit cameraState(m_cameraState);

    QRandomGenerator rg;

    while (m_run) {
        if (m_isPreviewEnabled) {
            if (!isStartedPreview) {
                isStartedPreview = true;
                m_cameraState = WzCameraState::PreviewStarted;
                emit cameraState(m_cameraState);
            } else {
                QMutexLocker lock(&m_mutexImageData);

                m_imageBytes = m_imageSize->width() * m_imageSize->height() * sizeof(uint16_t);
                rg.fillRange(reinterpret_cast<uint*>(m_imageData), m_imageBytes / sizeof(uint));
                uint16_t* pSrc = m_imageData;
                uchar* pDest = m_previewImageData;
                for(int j = 0; j < m_imageSize->width() * m_imageSize->height(); j++) {
                    *pDest = m_gray16to8Table[*pSrc];
                    pSrc++;
                    pDest++;
                }
                m_imageBytes = m_imageSize->width() * m_imageSize->height();
                m_pImageData = m_previewImageData;

                m_isPreviewUpdated = true;
            }

        } else {
            if (isStartedPreview) {
                isStartedPreview = false;
                m_cameraState = WzCameraState::PreviewStopped;
                emit cameraState(m_cameraState);
            }
        }

        if (m_isCapture) {
            // 模拟拍摄前的初始化
            if (!isInitCapture) {
                m_isPreviewEnabled = false;
                isStartedPreview = false;
                m_cameraState = WzCameraState::PreviewStopped;
                emit cameraState(m_cameraState);
                QThread::msleep(100);

                m_cameraState = WzCameraState::CaptureInit;
                emit cameraState(m_cameraState);
                isInitCapture = true;
                m_currentFrame = 0;
                m_capturedCount = 0;
                m_leftExposureMilliseconds = m_exposureMilliseconds[m_currentFrame];
                for (int i=0; i<20; i++) {
                    if (m_isAbortCapture) {
                        qDebug() << "初始化时终止了拍摄";
                        break;
                    }
                    QThread::msleep(50);
                }
                if (!m_isAbortCapture) {
                    m_cameraState = WzCameraState::Exposure;
                    emit cameraState(m_cameraState);
                }
            }
            if (m_isAbortCapture) {
                m_isAbortCapture = false;
                m_isCapture = false;
                isInitCapture = false;
                for (int i=0; i<20; i++) {
                    QThread::msleep(50);
                }
                m_cameraState = WzCameraState::CaptureAborted;
                emit cameraState(m_cameraState);
                continue;
            }            
            m_leftExposureMilliseconds -= 50;
            if (m_leftExposureMilliseconds <= 0) {
                m_leftExposureMilliseconds = 0;
                m_capturedCount++;
                if (m_currentFrame+1 == m_frameCount) {
                    m_isCapture = false;
                    m_isCaptureFinished = true;
                    isInitCapture = false;
                }

                m_cameraState = WzCameraState::Image;
                emit cameraState(m_cameraState);

                m_mutexImageData.lock();
                m_pImageData = m_imageData;
                QVariant binning = 1;
                getParam("Binning", binning);
                if (binning.toInt() > 4) binning = 4;
                if (binning.toInt() < 1) binning = 1;
                m_imageSize->setWidth(DEMO_CAMERA_WIDTH / binning.toInt());
                m_imageSize->setHeight(DEMO_CAMERA_HEIGHT / binning.toInt());
                m_imageBytes = m_imageSize->width() * m_imageSize->height() * sizeof(uint16_t);
                rg.fillRange(reinterpret_cast<uint*>(m_imageData), m_imageBytes / sizeof(uint));
                processImage();
                m_mutexImageData.unlock();

                m_cameraState = WzCameraState::CaptureFinished;
                emit cameraState(m_cameraState);
                QThread::msleep(50); // 必须停一下, 防止上层逻辑检测不到这个状态就变掉了

                if (m_currentFrame+1 < m_frameCount) {
                    m_currentFrame++;
                    m_leftExposureMilliseconds = m_exposureMilliseconds[m_currentFrame];
                    m_cameraState = WzCameraState::Exposure;
                    emit cameraState(m_cameraState);
                }
            }
        }

        QThread::msleep(50);
    }

    m_cameraState = WzCameraState::Disconnecting;
    emit cameraState(m_cameraState);

    QThread::msleep(1000);
    m_cameraState = WzCameraState::Disconnected;
    emit cameraState(m_cameraState);
}

WzAbstractCamera::WzAbstractCamera(QObject *parent):
    QObject(parent),
    m_cameraState(WzCameraState::Connecting),
    m_paramMutex(new QMutex())
{
}

WzAbstractCamera::~WzAbstractCamera() {
    delete m_paramMutex;
}

void WzAbstractCamera::setParam(const QString& paramKey, const QVariant& paramVal) {
    QMutexLocker lock(m_paramMutex);
    m_cameraParameters[paramKey] = paramVal;
}

bool WzAbstractCamera::getParam(const QString& paramKey, QVariant& paramVal) {
    QMutexLocker lock(m_paramMutex);
    if (!m_cameraParameters.contains(paramKey))
        return false;
    paramVal = m_cameraParameters[paramKey];
    return true;
}

void WzAbstractCamera::processImage() {

}

QString WzAbstractCamera::mkdirImagePath() {
    // TODO 需要判断路径是否为空, 如果没设置就要使用一个默认的路径, 比如exe所在目录
    // 但是这些逻辑不放在相机模块中，而是放在更上层的业务逻辑中。
    // 如果没设置过存储路径则选择剩余空间最多的一个分区建立 SHImages 目录
    // 建立失败则尝试在桌面建立，若仍然建立失败则弹出提示

    QVariant imagePath;
    getParam("ImagePath", imagePath);
    QDir path(imagePath.toString());
    path.mkpath(path.absolutePath());
    QString ymd = QDate::currentDate().toString("yyyy-MM-dd");
    path.mkdir(ymd);
    path.cd(ymd);
    return path.absolutePath();
}

QString WzAbstractCamera::getTiffFileName(const QString& path, const QDateTime &dateTime) {
    QVariant sampleName;
    getParam("SampleName", sampleName);
    QString fileName = sampleName.toString();
    if (fileName != "")
        fileName += "_";
    fileName += dateTime.toString("yyyy-MM-dd_hh.mm.ss");
    if (m_frameCount > 1) {
        fileName += "_" + QString::number(m_currentFrame + 1);
    }
    fileName += ".tif";
    QFileInfo tifFile(path, fileName);
    return tifFile.absoluteFilePath();
}

void WzAbstractCamera::newCameraState(const WzCameraState::CameraState &newCameraState)
{
    m_cameraState = newCameraState;
    emit cameraState(m_cameraState);
}

void WzAbstractCamera::emitPreviewImageUpdated()
{
    emit previewImageUpdated();
}

void WzAbstractCamera::emitCaptureFinished()
{
    emit captureFinished();
}

void rc(uint16_t* imageBuffer, const int& imageWidth, const int& imageHeight,
        const int& x, const int& y, uint16_t& rgb) {

    uint16_t* cc;

    if (x <= imageWidth - 1 && x >= 0 && y <= imageHeight - 1 && y >= 0) {
        cc = imageBuffer;
        cc = cc + y * imageWidth + x;
        rgb = *cc;
    } else {
        rgb = 0;
    }
}

uint16_t bilinear(uint16_t* imageBuffer, const int& imageWidth, const int& imageHeight,
                  const qreal& x, const qreal& y) {

    int j, k, rr;
    qreal cx, cy, m0, m1;
    uint16_t p0, p1, p2, p3;

    j = trunc(x);
    k = trunc(y);
    cx = x - floor(x);
    cy = y - floor(y);
    rc(imageBuffer, imageWidth, imageHeight, j, k, p0);

    rc(imageBuffer, imageWidth, imageHeight, j, k, p0);
    rc(imageBuffer, imageWidth, imageHeight, j + 1, k, p1);
    rc(imageBuffer, imageWidth, imageHeight, j, k + 1, p2);
    rc(imageBuffer, imageWidth, imageHeight, j + 1, k + 1, p3);
    m0 = p0 + cx * (p1 - p0);
    m1 = p2 + cx * (p3 - p2);
    rr = trunc(m0 + cy * (m1 - m0));
    return rr;
}

uint16_t *WzAbstractCamera::changeImageSize(uint16_t *imageBuffer, int &imageWidth, int &imageHeight, const qreal &zoom)
{
    qreal fx, fy, cxSrc, cySrc, cxDest, cyDest;
    int newWidth, newHeight;
    uint16_t *newImageBufferPtr, *newImageBuffer;

    qreal *arx, *ary;

    newWidth = round((imageWidth) * zoom);
    newHeight = round((imageHeight) * zoom);

    newImageBuffer = new uint16_t[newWidth * newHeight];
    memset(newImageBuffer, 0, newWidth * newHeight * sizeof(uint16_t));

    cxSrc = (imageWidth) / 2;
    cySrc = (imageHeight) / 2;
    cxDest = (newWidth) / 2;
    cyDest = (newHeight) / 2;

    arx = new qreal[newWidth];
    ary = new qreal[newHeight];

    for (int col = 0; col < newWidth; col++)
        arx[col] = cxSrc + (col - cxDest) / zoom;

    for (int row = 0; row < newHeight; row++)
        ary[row] = cySrc + (row - cyDest) / zoom;

    newImageBufferPtr = newImageBuffer;
    for (int row = 0; row < newHeight; row++) {
        for (int col = 0; col < newWidth; col++) {
            fx = arx[col];
            fy = ary[row];
            *newImageBufferPtr = bilinear(imageBuffer, imageWidth, imageHeight, fx, fy);
            newImageBufferPtr++;
        }
    }

    delete []ary;
    delete []arx;

    imageWidth = newWidth;
    imageHeight = newHeight;

    return newImageBuffer;
}

bool WzAbstractCamera::getMaxAndAvgGray(uint16_t *grayBuffer, int countOfPixel, uint16_t *grayAvg, uint16_t *maxGray, int64_t *allGray)
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

void WzAbstractCamera::setCameraCallback(WzCameraCallback* callback) {
    this->m_cameraCallback = callback;
}

int WzTestCamera::setPreviewEnabled(bool enabled)
{
    if (enabled) {
        QVariant binning = 1;
        getParam("Binning", binning);
        if (binning.toInt() > 4) binning = 4;
        if (binning.toInt() < 1) binning = 1;
        m_imageSize->setWidth(DEMO_CAMERA_WIDTH / binning.toInt());
        m_imageSize->setHeight(DEMO_CAMERA_HEIGHT / binning.toInt());
        m_imageBytes = m_imageSize->width() * m_imageSize->height();
        qDebug() << "setPreviewEnabled, enabled=" << enabled << ", binning=" << binning;
    }
    m_isPreviewEnabled = enabled;
    return ERROR_NONE;
}

void WzTestCamera::resetPreview()
{

}

int WzTestCamera::getImage(void* imageData)
{
    QMutexLocker lock(&m_mutexImageData);
    memcpy(imageData, m_pImageData, static_cast<size_t>(m_imageBytes));
    return ERROR_NONE;
}

int WzTestCamera::capture(const int exposureMilliseconds)
{
    int ms[] = {exposureMilliseconds};
    return capture(ms, 1);
}

int WzTestCamera::capture(const int* exposureMilliseconds, const int count)
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

void WzTestCamera::abortCapture() {
    qDebug() << "WzTestCamera::abortCapture(), m_isAbortCapture:" << m_isAbortCapture;
    if (m_isAbortCapture)
        return;
    m_isAbortCapture = true;
    m_cameraState = WzCameraState::CaptureAborting;
    emit cameraState(m_cameraState);
    qDebug() << "WzTestCamera::abortCapture()";
}

void WzTestCamera::handleTimerFired()
{
    if (m_isPreviewEnabled && m_isPreviewUpdated) {
        emit previewImageUpdated();
        m_isPreviewUpdated = false;
    } else if (m_isCaptureFinished) {
        emit captureFinished();
        m_isCaptureFinished = false;
    }
}

QSize* WzTestCamera::getImageSize() {
    return m_imageSize;
}

int WzTestCamera::getExposureMs() {
    return m_exposureMilliseconds[m_currentFrame];
}

int WzTestCamera::getLeftExposureMs() {
    return m_leftExposureMilliseconds;
}

int WzTestCamera::getCurrentFrame() {
    return m_currentFrame;
}

int WzTestCamera::getCapturedCount()
{
    return m_capturedCount;
}

int WzTestCamera::getImageBytes() {
    return m_imageBytes;
}

void WzTestCamera::processImage() {
    WzImageBuffer imageBuffer;
    imageBuffer.buf = reinterpret_cast<uint8_t*>(m_imageData);
    imageBuffer.width = static_cast<uint32_t>(m_imageSize->width());
    imageBuffer.height = static_cast<uint32_t>(m_imageSize->height());
    imageBuffer.bitDepth = 16;
    imageBuffer.bytesCountOfBuf = static_cast<uint32_t>(m_imageBytes);
    imageBuffer.exposureMs = getExposureMs();
    imageBuffer.update();

    QString path = mkdirImagePath();
    m_latestImageFile = getTiffFileName(path, QDateTime::currentDateTime());
    WzImageService::saveImageAsTiff(imageBuffer, m_latestImageFile);

    // TODO 测试代码
//    WzImageBuffer imageBufferExample;
//    WzImageService::loadImage("D:/PiPiLuu/code/_project/chemi/qt-project/chemi/example2.tif", &imageBufferExample);
//    AutoFreeArray af(imageBufferExample.bit16Array);
//    imageBufferExample.update();

    // 在当前路径下的 .sh_thumb 文件夹下生成同名缩略图
    m_latestThumbFile = WzImageService::createThumb(imageBuffer, m_latestImageFile);
}


QString WzTestCamera::getLatestImageFile() {
    return m_latestImageFile;
};

QString WzTestCamera::getLatestThumbFile() {
    return m_latestThumbFile;
}

double WzTestCamera::getTemperature() {
    return -30;
}

int WzTestCamera::getMarkerImage(uint16_t **imageData)
{
    QMutexLocker lock(&m_mutexImageData);
    if (nullptr != *imageData) {
        delete [] (*imageData);
        (*imageData) = nullptr;
    }
    setParam("imageWidth", m_imageSize->width());
    setParam("imageHeight", m_imageSize->height());
    (*imageData) = new uint16_t[m_imageSize->width() * m_imageSize->height()];
    memcpy(*imageData, m_imageData, m_imageBytes);
    return ERROR_NONE;
}
