#include "WzCaptureService.h"

WzCaptureService::WzCaptureService(QObject *parent):
    QObject(parent)
{
    qInfo() << "CaptureService";

    m_pCtlWs = new QWebSocket();
    QObject::connect(m_pCtlWs, &QWebSocket::stateChanged, this, &WzCaptureService::controlWsStateChanged);
    QObject::connect(m_pCtlWs, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &WzCaptureService::controlWsError);
    QObject::connect(m_pCtlWs, &QWebSocket::textMessageReceived, this, &WzCaptureService::controlWsTextMessage);

    m_pReconnectCameraTimer = new QTimer();
    m_pReconnectCameraTimer->setInterval(5000);
    m_pReconnectCameraTimer->setSingleShot(true);
    m_pReconnectCameraTimer->stop();
    QObject::connect(m_pReconnectCameraTimer, &QTimer::timeout, this, &WzCaptureService::reconnectCameraTimer);

    m_pImageTs = new QTcpSocket(this);
    m_imageStream.setDevice(m_pImageTs);
    m_imageStream.setVersion(QDataStream::Qt_5_12);

    m_camera = new WzNetCamera(m_pCtlWs);
    QObject::connect(qobject_cast<WzNetCamera*>(m_camera), &WzNetCamera::imageDownloaded, this, &WzCaptureService::imageLoaded);

        //m_previewImageSize(new QSize(100, 100)),
    m_captureTimer = new QTimer();

    QObject::connect(m_camera, SIGNAL(previewImageUpdated()), this, SLOT(previewImageUpdated()));
    QObject::connect(m_camera, SIGNAL(cameraState(WzCameraState::CameraState)), this, SLOT(cameraStateUpdated(WzCameraState::CameraState)));
    QObject::connect(m_captureTimer, SIGNAL(timeout()), this, SLOT(captureTimer()));

    m_captureTimer->setSingleShot(false);
    m_captureTimer->setInterval(900);

    m_camera->setCameraCallback(this);

    QObject::connect(&m_getAutoFocusStateTimer, &QTimer::timeout, this, &WzCaptureService::getAutoFocusStateTimer);
    m_getAutoFocusStateTimer.stop();
    m_getAutoFocusStateTimer.setSingleShot(false);
    m_getAutoFocusStateTimer.setInterval(10);
}

WzCaptureService::~WzCaptureService()
{
    m_getAutoFocusStateTimer.stop();

    if (nullptr != m_pReconnectCameraTimer) {
        delete m_pReconnectCameraTimer;
        m_pReconnectCameraTimer = nullptr;
    }
    if (nullptr != m_pCtlWs) {
        delete m_pCtlWs;
        m_pCtlWs = nullptr;
    }
    if (nullptr != m_pImageTs) {
        delete m_pImageTs;
        m_pImageTs = nullptr;
    }
    m_captureTimer->stop();
    delete m_captureTimer;
    //delete m_previewImageSize;
    if (m_previewImageData) {
        delete[] m_previewImageData;
        m_previewImageData = nullptr;
    }    
    delete m_camera;

    qInfo() << "~CaptureService";
}

bool WzCaptureService::enableCameraPreview(bool enabled, int binning) {
    if (enabled)
        m_camera->setParam("Binning", binning);
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
    m_captureState = Init;
    emit this->captureStateChanged(m_captureState);

    m_captureParams["action"] = "capture";
    m_captureParams["imagePath"] = m_imagePath;
    m_captureParams["isAutoExposure"] = m_isAutoExposure;

    m_captureTimer->start();
    m_camera->setParam("ImagePath", m_imagePath);
    m_camera->capture(qMax(1, params["exposureMs"].toInt()));
    return sendJson(QJsonObject::fromVariantMap(m_captureParams));
}

bool WzCaptureService::captureMulti(QVariantMap params) {
    qDebug() << "WzCaptureService::captureMulti, params:" << params;
    m_captureParams = params;

    m_captureState = Init;
    emit this->captureStateChanged(m_captureState);

    m_captureId = QDateTime::currentSecsSinceEpoch();

    m_captureParams["action"] = "captureMulti";
    m_captureParams["imagePath"] = m_imagePath;
    m_captureParams["isAutoExposure"] = m_isAutoExposure;

    QList<int> exposureMsList;
    if (params["multiType"] == "gray") {
        m_captureCount = params["grayAccumulate_frameCount"].toInt();
    } else if (params["multiType"] == "time") {
        m_captureCount = params["timeAccumulate_frameCount"].toInt();
        // 除了时间累积设置的帧数还包括基础时间的一帧，也就是说最少拍2帧
        m_captureCount++;
    } else if (params["multiType"] == "time_list") {
        m_captureCount = params["timeSequenceExposureMsList"].toList().count();
    } else {
        qWarning("无效的多帧拍摄参数");
        return false;
    }    

    m_camera->setParam("ImagePath", m_imagePath);

    m_captureTimer->start();
    m_camera->capture(nullptr, m_captureCount); // 这一句主要使相机实例内部的变量初始化
    return ERROR_NONE == sendJson(QJsonObject::fromVariantMap(m_captureParams));
}

void WzCaptureService::connectCamera() {

    if (m_serverAddress == "") {
        qWarning() << "Server address is nothing";
        return;
    }
    if (m_serverControlPort == 0) {
        qWarning() << "the port of camera control of server is zero";
        return;
    }
    QString url = QString("ws://%1:%2").arg(m_serverAddress).
            arg(m_serverControlPort);
    qDebug() << "connect" << url;
    m_pCtlWs->abort();
    m_pCtlWs->open(url);
    m_cameraState = WzCameraState::ConnectingServer;

    //m_pImageTs->abort();
    //m_pImageTs->connectToHost(m_serverAddress, m_serverImagePort);

    m_camera->setParam("serverAddress", m_serverAddress);
    m_camera->setParam("serverImagePort", m_serverImagePort);
    m_camera->setParam("serverFilePort", m_serverFilePort);
    m_camera->connect();
}

void WzCaptureService::disconnectCamera() {
    m_camera->disconnect();
}

void WzCaptureService::previewImageUpdated() {
    m_previewCount++;
    if (m_previewCount % 20 == 0)
        qDebug("WzCaptureService::previewImageUpdated, %lld", m_previewCount);

    int imageBytesCount = m_camera->getImageBytes();

    if (nullptr == m_previewImageData) {
        m_previewImageData = new uint8_t[imageBytesCount];
    } else if (m_previewImageBytesCount < imageBytesCount) {
        delete [] m_previewImageData;
        m_previewImageData = new uint8_t[imageBytesCount];
    }
    m_previewImageBytesCount = imageBytesCount;

    m_camera->getImage(m_previewImageData);

    if (g_liveImageView) {
        g_liveImageView->updateImage(m_previewImageData, m_previewImageBytesCount);
    }

    if (m_previewCount % 20 == 0)
        qDebug("WzCaptureService::previewImageUpdated, exit");

    emit previewImageRefreshed();
}

void WzCaptureService::captureTimer() {
    if (m_cameraState == WzCameraState::Exposure) {
        // 相机不支持异步拍摄
        if (m_camera->getLeftExposureMs() == -1) {
            m_exposurePercent = -1;
        } else {
            m_captureIndex = m_camera->getCurrentFrame();
            m_capturedCount = m_camera->getCapturedCount();
            m_captureCount = m_camera->getFrameCount();

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
        m_captureState = Exposure;
        emit captureStateChanged(m_captureState);
    }
}

void WzCaptureService::cameraStateUpdated(const WzCameraState::CameraState& state) {
    qDebug() << "CaptureService::cameraStateUpdated, " << state;
    if (m_cameraConnected != m_camera->connected()) {
        m_cameraConnected = m_camera->connected();
        emit cameraConnectedChanged(m_cameraConnected);
    }
    m_cameraState = state;
    switch(m_cameraState) {
    case WzCameraState::None:
        break;
    case WzCameraState::ConnectingServer:
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
        m_cameraConnected = true;
        break;
    case WzCameraState::AutoExposure:
        m_captureState = AutoExposure;
        emit captureStateChanged(m_captureState);
        break;
    case WzCameraState::AutoExposureFinished:
        m_captureState = AutoExposureFinished;
        emit captureStateChanged(m_captureState);
        break;
    case WzCameraState::CaptureInit:
        m_captureState = Init;
        emit captureStateChanged(m_captureState);
        break;
    case WzCameraState::CaptureAborting:
        break;
    case WzCameraState::CaptureAborted:
        m_captureTimer->stop();
        m_captureState = Aborted;
        emit captureStateChanged(m_captureState);
        break;
    case WzCameraState::Exposure: {
        captureTimer();
        if (!m_captureTimer->isActive())
            m_captureTimer->start();
        break;
    }
    case WzCameraState::Image:
        // TODO
        //m_camera->getLatestImageFile();
        m_captureIndex = m_camera->getCurrentFrame();
        m_captureState = Image;
        emit captureStateChanged(m_captureState);
        break;
    case WzCameraState::CaptureFinished: {
        m_cameraConnected = true;
        m_captureIndex = m_camera->getCurrentFrame();
        m_capturedCount = m_camera->getCapturedCount();

        QVariant imageInfo;
        if (!m_camera->getParam("imageInfo", imageInfo))
            return;

        if (!imageInfo.isValid())
            return;

        auto imageInfoMap = imageInfo.toMap();

        m_latestImageFile = imageInfoMap["imageFile"].toString();
        m_latestThumbFile = imageInfoMap["imageThumbFile"].toString();

        WzDatabaseService db;
        db.saveImage(imageInfo.toMap());

        if (m_capturedCount == m_captureCount) {
            m_captureTimer->stop();
        }
        m_captureState = Finished;
        emit captureStateChanged(m_captureState);
        break;
    }
    case WzCameraState::Disconnecting:
    case WzCameraState::Disconnected:
        m_cameraConnected = false;
        m_captureState = Aborted;
        emit captureStateChanged(m_captureState);
        m_pReconnectCameraTimer->start();
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

void WzCaptureService::controlWsStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "CaptureService::controlWsStateChanged";
    if (state == QAbstractSocket::ConnectedState) {
        m_pReconnectCameraTimer->stop();
        m_connectFailedCount = 0;        
        QJsonObject action;
        action["action"] = "readAdminParams";
        m_pCtlWs->sendTextMessage(QJsonDocument(action).toJson());
    } else if (state == QAbstractSocket::UnconnectedState) {
        if (m_pReconnectCameraTimer) {
            qInfo() << "CaptureService, control reconnect";
            m_pReconnectCameraTimer->start();
        }
    }
}

void WzCaptureService::controlWsError(QAbstractSocket::SocketError error)
{
    qInfo() << "CaptureService::controlWsError," << error;
}

void WzCaptureService::controlWsTextMessage(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull())
        return;

    QJsonValue action = doc["action"];
    if (action.isUndefined()) {}
    else if (action.toString() == "readAdminParams") {
       qDebug() << "action == readAdminParams";
       emit adminParamsReceived(doc["adminParams"].toObject());
    } else if (action.toString() == "getAutoFocusState") {
        if (m_isAutoFocusing != doc["isAutoFocusing"].toBool()) {
            m_isAutoFocusing = doc["isAutoFocusing"].toBool();
            emit isAutoFocusingChanged();
        }
    }

    if (!action.isUndefined()) {
        emit serverMessage(doc.object());
    }
}

void WzCaptureService::reconnectCameraTimer()
{
    m_connectFailedCount++;
    connectCamera();
}

int WzCaptureService::getConnectFailedCount() const
{
    return m_connectFailedCount;
}

int WzCaptureService::sendJson(const QJsonObject &object)
{
    return m_pCtlWs->sendTextMessage(QJsonDocument(object).toJson());
}

QString WzCaptureService::getServerAddress() const
{
    return m_serverAddress;
}

void WzCaptureService::setServerAddress(const QString &serverAddress)
{
    m_serverAddress = serverAddress;
    if (nullptr != m_camera) {
        m_camera->setParam("ServerAddress", serverAddress);
    }
}

bool WzCaptureService::cameraConnected()
{
    return m_cameraConnected;
}

bool WzCaptureService::getIsAutoFocusing() const
{
    return m_isAutoFocusing;
}

void WzCaptureService::getAutoFocusStateTimer()
{
    QJsonObject action;
    action["action"] = "getAutoFocusState";
    sendJson(action);
}

void WzCaptureService::serverHostNameChanged(const QString &hostName)
{
    m_serverAddress = hostName;
    m_mcuPort = WzUdpBroadcastReceiver::McuPort;
    m_serverImagePort = WzUdpBroadcastReceiver::PreviewPort;
    m_serverControlPort = WzUdpBroadcastReceiver::CameraPort;
    m_serverFilePort = WzUdpBroadcastReceiver::FilePort;
    emit serverAddressChanged(hostName);
}

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
    //return m_camera->
    return 0;
}

void WzCaptureService::setPreviewExposureMs(int exposureMs) {
    m_camera->setParam("ExposureMs", exposureMs);
}

bool WzCaptureService::getMarkerImage()
{
    return m_camera->getMarkerImage(nullptr) > 0;
}

void WzCaptureService::clearMarkerImage()
{
    // TODO

};

void WzCaptureService::saveAdminSetting(const QString &params)
{
    qDebug() << "CaptureService::saveAdminSetting";
    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8());
    if (doc.isNull())
        return;

    QJsonObject action;
    action["action"] = "saveAdminParams";
    action["params"] = doc.object();

    sendJson(action);
}

void WzCaptureService::startAutoFocus(const QRect &focusRect)
{
    m_isAutoFocusing = true;
    emit isAutoFocusingChanged();
    QJsonObject action;
    action["action"] = "startAutoFocus";
    action["focusRectLeft"] = focusRect.left();
    action["focusRectRight"] = focusRect.right();
    action["focusRectTop"] = focusRect.top();
    action["focusRectBottom"] = focusRect.bottom();
    action["isGel"] = WzUtils::isGelCapture();
    sendJson(action);
    m_getAutoFocusStateTimer.start();
}

void WzCaptureService::stopAutoFocus()
{
    QJsonObject action;
    action["action"] = "stopAutoFocus";
    sendJson(action);
}

void WzCaptureService::openUdp(const int port)
{
    qInfo() << "CaptureService::openUdp";
    qInfo() << "\tPort:" << port;
    if (m_udpBroadcastReceiver) {
        qInfo() << "\tThe udp port is already open";
        return;
    }
    m_udpBroadcastReceiver = new WzUdpBroadcastReceiver(port, this);
    connect(m_udpBroadcastReceiver, &WzUdpBroadcastReceiver::serverHostNameChanged,
            this, &WzCaptureService::serverHostNameChanged);
};

QString WzCaptureService::saveMarkerImage(const QDateTime &dateTime) {
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
        qDebug() << "saveMarkerImage, pixel[100,100] == 0";
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

void WzCaptureService::readCameraSNList() {
    return;
}

QString WzCaptureService::imagePath() {
    return m_imagePath;
};

void WzCaptureService::setImagePath(const QString imagePath) {
    m_imagePath = imagePath;
};

WzCameraState::CameraState WzCaptureService::cameraState() {
    return m_cameraState;
}

bool WzCaptureService::getAutoExposure() {
    return m_isAutoExposure;
};

void WzCaptureService::setAutoExposure(bool autoExposure) {
    m_isAutoExposure = autoExposure;
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

int WzCaptureService::getAutoExposureMs() {
    if (m_camera != nullptr)
        return m_camera->getExposureMs();
    else
        return 0;
}
