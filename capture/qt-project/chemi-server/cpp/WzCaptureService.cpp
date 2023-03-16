#include "WzCaptureService.h"

WzCaptureService::WzCaptureService(QObject *parent):
    QObject(parent),
#ifdef MAC
    m_camera(new WzTestCamera(parent)),
#endif
    m_previewImageData(nullptr),
    m_previewImageSize(new QSize(100, 100)),
    m_captureTimer(new QTimer()),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("/camera/control"), QWebSocketServer::NonSecureMode))
{
    qDebug() << "WzCaptureService";

#ifdef KSJ_CAMERA
    m_cameras.append(new WzKsjCamera(parent));
#endif
#ifdef PVCAM
    m_cameras.append(new WzPvCamera(parent));
#endif
#ifdef ATIK
    m_cameras.append(new WzAtikCamera(parent));
#endif
#ifdef DEMO
    m_cameras.append(new WzTestCamera(parent));
#endif

    readCameraSNList();
    readPorts();

    foreach(WzAbstractCamera *camera, m_cameras) {
        QObject::connect(camera, &WzAbstractCamera::cameraCount, this, &WzCaptureService::cameraCountSlot);
        QObject::connect(camera, &WzAbstractCamera::previewImageUpdated, this, &WzCaptureService::previewImageUpdated);
        QObject::connect(camera, &WzAbstractCamera::cameraState, this, &WzCaptureService::cameraStateUpdated);
        camera->setCameraCallback(this);
    }

    QObject::connect(m_captureTimer, SIGNAL(timeout()), this, SLOT(captureTimer()));
    m_captureTimer->setSingleShot(false);
    m_captureTimer->setInterval(900);

    //m_camera->setCameraCallback(this);

    m_previewServer = new WzTcpServer();
    m_previewServer->listen(QHostAddress::Any, m_previewServerPort);
    QObject::connect(m_previewServer, &WzTcpServer::clientDisconnected, this, &WzCaptureService::tcpClientDisconnected);

    m_fileServer = new WzFileServer();
    bool ret = m_fileServer->listen(QHostAddress::Any, m_fileServerPort);
    qDebug() << "FileServer listen " << m_fileServerPort << ", " << ret;

    m_fileServer2 = new ShstFileServer();
    if (m_fileServer2->listen()) {
        WzUdpBroadcastSender::FilePort2 = m_fileServer2->serverPort();
        qInfo() << "FileServer2 listening on" << WzUdpBroadcastSender::FilePort2;
    } else {
        qWarning() << "FileServer2 listen failure";
    }

    for(int i = 0; i < 256; ++i) m_colorTable.append(qRgb(i,i,i));

    connect(&m_autoFocus, &WzAutoFocus::focusFar, this, &WzCaptureService::autoFocusFar);
    connect(&m_autoFocus, &WzAutoFocus::focusNear, this, &WzCaptureService::autoFocusNear);
    connect(&m_autoFocus, &WzAutoFocus::focusStop, this, &WzCaptureService::autoFocusStopSlot);
    connect(&m_autoFocus, &WzAutoFocus::getImage, this, &WzCaptureService::autoFocusGetImage);
    connect(&m_autoFocus, &WzAutoFocus::log, this, &WzCaptureService::autoFocusLog);
    connect(&m_autoFocus, &WzAutoFocus::finished, this, &WzCaptureService::autoFocusFinishedSlot);

    m_udpBroadcastSender = new WzUdpBroadcastSender(m_udpBroadcastPort, this);
}

WzCaptureService::~WzCaptureService()
{
    if (nullptr != m_fileServer2) {
        delete m_fileServer2;
        m_fileServer2 = nullptr;
    }
    if (nullptr != m_fileServer) {
        delete m_fileServer;
        m_fileServer = nullptr;
    }
    if (nullptr != m_previewServer) {
        delete m_previewServer;
        m_previewServer = nullptr;
    }

    if (nullptr != m_previewImage) {
        delete m_previewImage;
        m_previewImage = nullptr;
    }

    m_captureTimer->stop();
    delete m_captureTimer;
    delete m_previewImageSize;
    if (m_previewImageData) {
        delete[] m_previewImageData;
        m_previewImageData = nullptr;
    }

    foreach(WzAbstractCamera *camera, m_cameras) {
        delete camera;
    }
}

void WzCaptureService::initCameras()
{

}

bool WzCaptureService::enableCameraPreview(bool enabled, int binning) {
    updateCameraParam();
    if (m_camera) {
        if (enabled)
            m_camera->setParam("Binning", binning);
        return m_camera->setPreviewEnabled(enabled);
    } else {
        return false;
    }
}

bool WzCaptureService::resetPreview()
{
    if (m_camera) {
        m_camera->resetPreview();
        return true;
    } else {
        return false;
    }
}

bool WzCaptureService::capture(QVariantMap params) {
    m_captureParams = params;
    updateCameraParam();

    qDebug() << "WzCaptureService::capture, params:" << params;
    m_captureId = 0;
    m_captureCount = 1;
    m_captureIndex = 0;
    newCaptureState(Init);
    m_camera->setParam("ImagePath", m_imagePath);
    m_camera->setParam("Binning", params["binning"]);
    m_camera->setParam("AE", m_isAutoExposure);
    m_camera->setParam("SampleName", params["sampleName"]);
    m_camera->setParam("grayAccumulate", params["grayAccumulate"]);

    QString lt = "";
    if (params.contains("openedLightType"))
        lt = params["openedLightType"].toString();
    if (params["isLightOpened"].toBool() &&
            (lt == "uv_penetrate" || lt == "uv_penetrate_force" ||
             lt == "white_down" || lt == "red" || lt == "green" || lt == "blue")) {
        m_camera->setParam("isThumbNegative", false);
    } else {
        m_camera->setParam("isThumbNegative", true);
    }
    // 来自客户端的参数优先级最高
    if (params.contains("isThumbNegative")) {
        m_camera->setParam("isThumbNegative", params["isThumbNegative"]);
    }

    m_elapsedExposureTime = "";
    m_elapsedExposureMs = 0;
    m_exposureMs = params["exposureMs"].toInt();
    m_leftExposureTime = m_exposureMs;
    m_leftExposureMs = m_exposureMs;

    bool ret = m_camera->capture(qMax(1, params["exposureMs"].toInt()));
    m_captureTimer->start();
    return ret;
}

bool WzCaptureService::captureMulti(QVariantMap params) {
    qDebug() << "WzCaptureService::captureMulti, params:" << params;
    m_captureParams = params;
    updateCameraParam();

    newCaptureState(Init);

    m_captureId = QDateTime::currentSecsSinceEpoch();
    m_camera->setParam("ImagePath", m_imagePath);
    m_camera->setParam("Binning", params["binning"].toInt());
    m_camera->setParam("AE", m_isAutoExposure);
    m_camera->setParam("SampleName", params["sampleName"]);

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
    if (m_webSocketServerPort == 0) {
        qWarning() << "CaptureService, listen port is zero";
    } else {
        if (m_pWebSocketServer->listen(QHostAddress::Any, m_webSocketServerPort)) {
            connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                    this, &WzCaptureService::onWebSocketNewConnection);
        } else {
            qWarning() << "CaptureService, listen failure";
        }
    }
    // 按照设计, init 和 connect 应该是分开的, 但为了赶时间暂时先放到一起 at 2020-12-12
    foreach(WzAbstractCamera *camera, m_cameras) {
        camera->connect();
    }
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

            if (nullptr != m_previewImage) {
                delete m_previewImage;
                m_previewImage = nullptr;
            }
        }

        if (nullptr == m_previewImageData) {
            m_previewImageData = new uchar[imageSize->width() * imageSize->height()];
        }

        m_camera->getImage(m_previewImageData);
    }

    if (nullptr == m_previewImage) {
        m_previewImage = new QImage(*m_previewImageSize, QImage::Format_Indexed8);
        m_previewImage->setColorTable(m_colorTable);
    }
    for (int row = 0; row < m_previewImage->height(); row++) {
        auto line = m_previewImage->scanLine(row);
        memcpy(line, &m_previewImageData[row * imageSize->width()], imageSize->width());
    }


    QByteArray ba;
    QBuffer buf(&ba);
#ifdef DEMO
    m_previewImage->save(&buf, "JPEG", 10);
#else
    m_previewImage->save(&buf, "JPEG", 70);
#endif
    m_previewServer->writeData(ba);

    m_markerImageSize.setWidth(imageSize->width());
    m_markerImageSize.setHeight(imageSize->height());

    if (g_liveImageView) {
        //g_liveImageView->updateImage(m_previewImageData, *imageSize);
    }

    if (m_previewCount % 20 == 0)
        qDebug("WzCaptureService::previewImageUpdated, exit");

    m_isPreviewUpdated = true;
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
            m_leftExposureMs = m_camera->getLeftExposureMs();
            m_leftExposureTime = WzUtils::getTimeStr(m_leftExposureMs, false);
            m_elapsedExposureMs = exposureMilliseconds - m_camera->getLeftExposureMs();
            m_elapsedExposureTime = WzUtils::getTimeStr(m_elapsedExposureMs, false);
        }
        newCaptureState(WzCaptureService::Exposure);
    }
}

void WzCaptureService::cameraStateUpdated(const WzCameraState::CameraState& state) {
    // 如果相机已连接而且触发此次信号的不是连接的相机则忽略信号
    if (m_camera && m_camera != QObject::sender())
        return;
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
        break;
    case WzCameraState::AutoExposure:
        newCaptureState(WzCaptureService::AutoExposure);
        break;
    case WzCameraState::AutoExposureFinished:
        newCaptureState(WzCaptureService::AutoExposureFinished);
        break;
    case WzCameraState::CaptureInit:
        m_isCaptureFinished = false;
        newCaptureState(WzCaptureService::Init);
        break;
    case WzCameraState::CaptureAborting:
        break;
    case WzCameraState::CaptureAborted:
        m_captureTimer->stop();
        newCaptureState(WzCaptureService::Aborted);
        break;
    case WzCameraState::Exposure: {
        captureTimer();
        break;
    }
    case WzCameraState::Image:
        // TODO
        //m_camera->getLatestImageFile();
        m_captureIndex = m_camera->getCurrentFrame();
        newCaptureState(WzCaptureService::Image);
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

        QVariant imageWidth, imageHeight;
        m_camera->getParam("imageWidth", imageWidth);
        m_camera->getParam("imageHeight", imageHeight);
        imageInfo["imageWidth"] = imageWidth;
        imageInfo["imageHeight"] = imageHeight;

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

        // 客户端设置的变量优先级高于服务端的逻辑
        if (m_captureParams.contains("isSaveMarker")) {
            isSaveMarker = m_captureParams["isSaveMarker"].toBool();
        }
        if (m_captureParams.contains("imageInvert"))
            imageInfo["imageInvert"] = m_captureParams["imageInvert"];

        if (isSaveMarker)
            imageInfo["imageWhiteFile"] = saveMarkerImage(captureDateTime.toDateTime());

        db.saveImage(imageInfo);
        m_imageInfo = imageInfo;

        if (m_capturedCount == m_captureCount) {
            m_isCaptureFinished = true;
            m_captureTimer->stop();
        }

        newCaptureState(WzCaptureService::Finished);

        //autoClearImages(db);
        break;
    }
    case WzCameraState::Disconnecting:
    case WzCameraState::Disconnected:
        m_cameraConnected = false;
        break;
    case WzCameraState::CameraNotFound:
        return; // 在 cameraCount 中处理
    case WzCameraState::Error:
        break;
    }
    newCameraState(state);
}

void WzCaptureService::autoFocusGetImage()
{
    QMutexLocker lock(&m_previewImageDataLock);
    m_autoFocus.addFrame(m_previewImageData, *m_previewImageSize, m_autoFocusRect);
}

void WzCaptureService::autoFocusLog(const QString &msg)
{
    qDebug() << "AF:" << msg;
}

void WzCaptureService::autoFocusStopSlot()
{
    m_isAutoFocusing = false;
    emit autoFocusStop();
}

void WzCaptureService::autoFocusFinishedSlot()
{
    m_isAutoFocusing = false;
    emit autoFocusFinished();
}

void WzCaptureService::onWebSocketNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WzCaptureService::textMessageReceived);
    connect(pSocket, &QWebSocket::disconnected, this, &WzCaptureService::onWebSocketDisconnected);

    m_clients << pSocket;
}

void WzCaptureService::textMessageReceived(const QString &message)
{
    //qDebug() << "Message received:" << message;

    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());

    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if (jsonDoc.isNull())
        return;
    QJsonValue action = jsonDoc["action"];
    if (action.isUndefined())
        return;

    QJsonObject response;

    QString act = action.toString();
    if (act == "getCameraInfo") {

        response["action"] = action;
        QVariantList stateList;
        for (int i = 0; i < m_cameraStateList.count(); i++)
            stateList << m_cameraStateList[i];        
        response["m_cameraState"] = QJsonValue::fromVariant(stateList);
        m_cameraStateList.clear();
        if (nullptr != m_camera) {
            response["m_currentFrame"] = m_camera->getCurrentFrame();
            response["m_isPreviewEnabled"] = m_cameraState == WzCameraState::PreviewStarted;
            response["m_isCaptureFinished"] = m_isCaptureFinished;
            response["m_capturedCount"] = m_capturedCount;
            response["m_exposureMilliseconds"] = m_camera->getExposureMs();
            response["m_leftExposureMilliseconds"] = m_camera->getLeftExposureMs();
            response["m_latestImageFile"] = m_camera->getLatestImageFile();
            response["m_latestThumbFile"] = m_camera->getLatestThumbFile();
            response["m_temperature"] = m_camera->getTemperature();
            response["m_captureCount"] = m_captureCount;
        }
        response["m_cameraConnected"] = m_cameraConnected;
        response["m_cameraState2"] = m_cameraState;        

    } else if (act == "setPreviewEnabled") {

        if (nullptr != m_camera) {
            m_camera->setParam("Binning", jsonDoc["binning"].toInt());
            m_camera->setPreviewEnabled(jsonDoc["enabled"].toBool());
        }

    } else if (act == "resetPreview") {

        if (nullptr != m_camera) {
            QJsonValue exposureMs = jsonDoc["exposureMs"];
            if (!exposureMs.isUndefined())
                m_camera->setParam("ExposureMs", exposureMs.toInt());
            m_camera->resetPreview();
        }

    } else if (act == "capture") {

        //m_imagePath = jsonDoc["imagePath"].toString();
        m_isAutoExposure = jsonDoc["isAutoExposure"].toBool();
        capture(jsonDoc.toVariant().toMap());

    } else if (act == "captureMulti") {

        //m_imagePath = jsonDoc["imagePath"].toString();
        m_isAutoExposure = jsonDoc["isAutoExposure"].toBool();
        captureMulti(jsonDoc.toVariant().toMap());

    } else if (act == "abortCapture") {

        abortCapture();

    } else if (act == "getFile") {

        sendFileToClient(jsonDoc["fileName"].toString());

    } else if (act == "getMarkerImage") {

        response["action"] = action;
        response["code"] = getMarkerImage(jsonDoc["avgGrayThreshold"].toInt());

    } else if (act == "readAdminParams") {

        QVariantMap params = WzSetting2::readAdminSetting();
        params["captureState"] = m_captureState;

        response["action"] = action;
        response["adminParams"] = QJsonValue::fromVariant(params);

    } else if (act == "saveAdminParams") {
        QVariant params = jsonDoc["params"].toVariant();
        WzSetting2::saveAdminSetting(params.toMap());
    }

    else if (act == "startAutoFocus") {
        m_autoFocusRect.setLeft  (jsonDoc["focusRectLeft"].toInt());
        m_autoFocusRect.setRight (jsonDoc["focusRectRight"].toInt());
        m_autoFocusRect.setTop   (jsonDoc["focusRectTop"].toInt());
        m_autoFocusRect.setBottom(jsonDoc["focusRectBottom"].toInt());
        m_isGel = jsonDoc["isGel"].toBool();
        startAutoFocus();
    } else if (act == "stopAutoFocus") {
        stopAutoFocus();
    } else if (act == "getAutoFocusState") {
        response["action"] = action;
        response["isAutoFocusing"] = m_isAutoFocusing;
    }


    if (pClient && !response.isEmpty()) {
        pClient->sendTextMessage(QJsonDocument(response).toJson());
    }
}

bool WzCaptureService::getIsGel() const
{
    return m_isGel;
}

void WzCaptureService::readPorts()
{
    auto readPort = [](const QProcessEnvironment &pe, const QString &key, int &port) {
        qInfo() << "Read port," << key;
        if (pe.contains(key)) {
            qInfo() << "\tcontains true";
            QString portStr = pe.value(key);
            int p;
            bool ok;
            p = portStr.toInt(&ok);
            if (ok) {
                port = p;
                qInfo() << "\tport is" << port;
            }
        } else {
            qInfo() << "\t" << "contains false";
        }
    };
    QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();
    readPort(pe, "SHSTServerPreviewPort", m_previewServerPort);
    readPort(pe, "SHSTServerCameraPort", m_webSocketServerPort);
    readPort(pe, "SHSTServerFilePort", m_fileServerPort);
    readPort(pe, "SHSTServerUdpBroadcastPort", m_udpBroadcastPort);
    WzUdpBroadcastSender::PreviewPort = m_previewServerPort;
    WzUdpBroadcastSender::CameraPort = m_webSocketServerPort;
    WzUdpBroadcastSender::FilePort = m_fileServerPort;
}

void WzCaptureService::onWebSocketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    qDebug() << "webSocketDisconnected:" << pClient;
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
    if (m_clients.count() == 0) {
        // 2021-2-25
        // Android客户端在屏幕关闭后进入休眠状态，此时也不会断开WebSocket连接，所以此处不再
        // 暂停预览
    }
}

int WzCaptureService::getListenPort() const
{
    return m_webSocketServerPort;
}

void WzCaptureService::setListenPort(int listenPort)
{
    m_webSocketServerPort = listenPort;
}

void WzCaptureService::cameraCountSlot(WzAbstractCamera *sender, const int count)
{
    Q_UNUSED(sender)
    qInfo() << "CaptureService::cameraCount, " << count;
    m_cameraCount += count;
    m_searchedCameraTypeCount++;
    if (m_searchedCameraTypeCount == m_cameras.count()) {
        if (m_cameraCount == 0)
            newCameraState(WzCameraState::CameraNotFound);
        emit cameraCountChanged(m_cameraCount);
    }
    if (count > 0)
        m_camera = sender;
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
    return m_cameraCount;
}

void WzCaptureService::setPreviewExposureMs(int exposureMs) {
    if (m_camera)
        m_camera->setParam("ExposureMs", exposureMs);
}

int WzCaptureService::getMarkerImage(const int avgGrayThreshold)
{
    qInfo() << "CaptureService::getMarkerImage";

    if (nullptr != m_markerTiff16bit) {
        delete [] m_markerTiff16bit;
        m_markerTiff16bit = nullptr;
    }

    int ret = m_camera->getMarkerImage(&m_markerTiff16bit);
    qInfo() << "\t result " << ret;

    if (ret > 0) {
        auto imageSize = m_camera->getImageSize();
        // 一个宽度X像素高度Y像素的方块，且位于图片中央, 如果其平均灰阶过低可能是没开白光,
        // 返回错误代码
        const int blockWidth = 300;
        const int blockHeight = 50;
        if (imageSize->width() < blockWidth || imageSize->height() < blockHeight) {
            return -4;
        }
        int startRow = (imageSize->height() - blockHeight) / 2;
        int startCol = (imageSize->width() - blockWidth) / 2;
        uint64_t grays = 0;
        for (int row = startRow; row < startRow + blockHeight; row++) {
            for (int col = startCol; col < startCol + blockWidth; col++) {
                grays += m_markerTiff16bit[row * imageSize->width() + col];
            }
        }
        uint64_t avgGray = grays / (blockWidth * blockHeight);
        // 平均灰阶小于特定值可能是没开光源或者画面太暗
        qInfo() << "\tg:" << grays << avgGray;
        if (avgGray < static_cast<uint64_t>(avgGrayThreshold)) {
            return -3;
        }
    }

    return ret;
}

void WzCaptureService::startAutoFocus()
{
    m_isAutoFocusing = true;
    m_autoFocus.start();
}

void WzCaptureService::stopAutoFocus()
{
    m_isAutoFocusing = false;
    m_autoFocus.stop();
}

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
    if (RY3_SUCCESS != RY3_Open(&handle, 1)) return;

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
    memcpy(key, &outBuf[55], 32);

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
    if (m_camera)
        m_camera->setParam(paramName, paramVal);
    m_cameraParams[paramName] = paramVal;
}

int WzCaptureService::getAutoExposureMs() {
    if (m_camera != nullptr)
        return m_camera->getExposureMs();
    else
        return 0;
}

void WzCaptureService::sendJson(const QJsonObject &json)
{
    for (int i = 0; i < m_clients.count(); i++) {
        QWebSocket *pClient = m_clients.at(i);
        if (pClient)
            pClient->sendTextMessage(QJsonDocument(json).toJson());
    }
}

void WzCaptureService::sendFileToClient(const QString &fileName)
{
    QFile f(fileName);
    f.open(QFile::ReadOnly);
    m_previewServer->writeData(f.readAll());
    f.close();
}

void WzCaptureService::newCaptureState(WzCaptureService::CaptureState state)
{
    m_captureState = state;
    emit this->captureState(state);
}

void WzCaptureService::newCameraState(const WzCameraState::CameraState &state)
{
    m_cameraState = state;
    emit cameraStateChanged(state);
    QVariantMap map;
    map["cameraState"] = state;
    if (state == WzCameraState::CaptureFinished) {
        map["imageInfo"] = m_imageInfo;
    }
    m_cameraStateList << map;
}

void WzCaptureService::updateCameraParam()
{
    if (m_camera) {
        QMapIterator<QString, QVariant> iter(m_cameraParams);
        while (iter.hasNext()) {
            iter.next();
            m_camera->setParam(iter.key(), iter.value());
        }
    }
}

void WzCaptureService::autoClearImages(WzDatabaseService &dbService)
{
    int diskFreeSpaceLimit = dbService.readIntOption("diskFreeSpaceLimit", 500);
    int diskFreeSpaceTarget = dbService.readIntOption("diskFreeSpaceTarget", 1000);

    WzDiskUtils du;
    if (du.diskFreeSpaceLessThan(m_imagePath, diskFreeSpaceLimit)) {
        du.clearHistoryImages(dbService, m_imagePath, diskFreeSpaceTarget);
    }
}

/*
QJsonObject WzCaptureService::captureByNetCamera(const QJsonDocument &json)
{
    QJsonObject result;
    result["action"] = json["action"];

    QJsonArray exposureMs = json["exposureMs"].toArray();
    if (exposureMs.count() == 0) {
        result["code"] = QJsonValue::fromVariant(ERROR_PARAM);
        qWarning() << "capture, no exposure time";
        return result;
    }

    int *exposureMsInt = new int[exposureMs.count()];
    for (int i = 0; i < exposureMs.count(); i++)
        exposureMsInt[i] = exposureMs.at(i).toInt();

    auto cameraParams = json["cameraParams"].toObject().toVariantMap();
    auto iter = cameraParams.constBegin();
    while (iter != cameraParams.constEnd()) {
        m_camera->setParam(iter.key(), iter.value());
        iter++;
    }

    // 此参数可能被覆盖所以重新设置一次
    m_camera->setParam("ImagePath", m_imagePath);

    m_camera->capture(exposureMsInt, exposureMs.count());

    delete [] exposureMsInt;

    result["code"] = QJsonValue::fromVariant(ERROR_NONE);
    return result;
}
*/
