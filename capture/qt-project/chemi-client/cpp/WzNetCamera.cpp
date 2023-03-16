#include "WzNetCamera.h"

WzNetCamera::WzNetCamera(QObject *parent) : WzAbstractCamera(nullptr)
{  
    m_pGetCameraInfoTimer = new QTimer();
    QObject::connect(m_pGetCameraInfoTimer, &QTimer::timeout, this, &WzNetCamera::getCameraInfoTimer);
    m_pGetCameraInfoTimer->setInterval(100);
    m_pGetCameraInfoTimer->setSingleShot(false);

    m_pCtlWs = qobject_cast<QWebSocket*>(parent);
    QObject::connect(m_pCtlWs, &QWebSocket::textMessageReceived, this, &WzNetCamera::controlWsTextMessage);
    QObject::connect(m_pCtlWs, &QWebSocket::stateChanged, this, &WzNetCamera::controlWsStateChanged);
    QObject::connect(m_pCtlWs, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &WzNetCamera::controlWsError);

    m_pImageTs = new QTcpSocket(this);
    m_tcpStream.setDevice(m_pImageTs);
    m_tcpStream.setVersion(QDataStream::Qt_5_12);

    QObject::connect(m_pImageTs, &QIODevice::readyRead, this, &WzNetCamera::readImageFromServer);
    QObject::connect(m_pImageTs, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &WzNetCamera::imageSocketError);
}

WzNetCamera::~WzNetCamera()
{
    if (nullptr != m_pGetCameraInfoTimer) {
        m_pGetCameraInfoTimer->stop();
        delete m_pGetCameraInfoTimer;
        m_pGetCameraInfoTimer = nullptr;
    }
    if (nullptr != m_previewImageData) {
        delete [] m_previewImageData;
        m_previewImageData = nullptr;
    }
}

void WzNetCamera::connect()
{
    if (m_isConnected)
        return;

    m_ctlWsState = m_pCtlWs->state();
    if (m_ctlWsState == QAbstractSocket::ConnectedState) {
        m_pGetCameraInfoTimer->start();
    } else {
        m_cameraState = WzCameraState::ConnectingServer;
        emit cameraState(m_cameraState);
    }    
}

void WzNetCamera::disconnect()
{    
    if (!m_isConnected)
        return;

    m_pGetCameraInfoTimer->stop();
    m_pImageTs->disconnectFromHost();
    //m_pCtlWs->close();
    m_cameraState = WzCameraState::Disconnected;
    m_isConnected = false;
    emit cameraState(m_cameraState);
}

void WzNetCamera::run()
{
    QJsonObject action;
    action["action"] = "getCameraInfo";
    int sentBytes = m_pCtlWs->sendTextMessage(QJsonDocument(action).toJson());

    //qDebug() << "sentBytes," << sentBytes;

    // 如果已经连接成功而且3秒钟收不到服务器信息说明断开了, 具体有几种可能
    // 1. 服务端软件崩溃
    // 2. 服务器硬件关机
    // 3. 本机或服务器网络断开
    if (m_isConnected &&
            QDateTime::currentDateTime() > m_latestReceivedServerMsgTime.addSecs(3)) {

    }
}

int WzNetCamera::setPreviewEnabled(bool enabled)
{
    if (enabled)
        connectImageServer();
    else
        m_isPreviewEnabled = 0;
    QJsonObject action;
    action["action"] = "setPreviewEnabled";
    action["enabled"] = enabled;
    action["binning"] = m_cameraParameters["Binning"].toInt();
    return sendJson(action);
}

void WzNetCamera::resetPreview()
{
    QJsonObject action;
    action["action"] = "resetPreview";

    QVariant exposureMs;
    getParam("ExposureMs", exposureMs);
    action["exposureMs"] = exposureMs.toInt();

    sendJson(action);
}

QSize *WzNetCamera::getImageSize()
{
    return &m_imageSize;
}

int WzNetCamera::getImageBytes()
{
    return m_previewImageDataSize;
}

int WzNetCamera::getImage(void *imageData)
{
    QMutexLocker lock(&m_mutexImageData);
    memcpy(imageData, m_previewImageData, m_previewImageDataSize);
    return m_previewImageDataSize;
}

bool WzNetCamera::connected()
{
    return m_isConnected;
}

int WzNetCamera::capture(const int exposureMilliseconds)
{
    int ms[] = {exposureMilliseconds};
    return capture(ms, 1);
}

int WzNetCamera::capture(const int *exposureMilliseconds, const int count)
{
    Q_UNUSED(exposureMilliseconds)
    if (count == 0)
        return ERROR_PARAM;
    m_frameCount = count;
    m_isAbortCapture = false;
    m_isCapture = true;
    m_capturedCount = 0;
    return ERROR_NONE;
}

void WzNetCamera::abortCapture()
{
    qDebug() << "NetCamera::abortCapture, m_isAbortCapture:" << m_isAbortCapture;
    if (m_isAbortCapture) {
        qInfo() << "m_isAbortCapture == true";
        return;
    }
    m_isAbortCapture = true;
    m_cameraState = WzCameraState::CaptureAborting;
    emit cameraState(m_cameraState);

    QJsonObject action;
    action["action"] = "abortCapture";

    qDebug() << "NetCamera::abortCapture," << action;

    sendJson(action);
}

int WzNetCamera::getExposureMs()
{
    return m_exposureMilliseconds;
}

int WzNetCamera::getLeftExposureMs()
{
    return m_leftExposureMilliseconds;
}

int WzNetCamera::getCurrentFrame()
{
    return m_currentFrame;
}

int WzNetCamera::getCapturedCount()
{
    return m_capturedCount;
}

QString WzNetCamera::getLatestImageFile()
{
    return m_latestImageFile;
}

QString WzNetCamera::getLatestThumbFile()
{
    return m_latestThumbFile;
}

double WzNetCamera::getTemperature()
{
    return m_temperature;
}

int WzNetCamera::getMarkerImage(uint16_t **buffer)
{
    Q_UNUSED(buffer)
    QVariant markerAvgGrayThreshold;
    getParam("markerAvgGrayThreshold", markerAvgGrayThreshold);
    qInfo() << "getMarkerImage," << markerAvgGrayThreshold.toInt();
    QJsonObject action;
    action["action"] = "getMarkerImage";
    action["avgGrayThreshold"] = markerAvgGrayThreshold.toInt();
    sendJson(action);
    return ERROR_NONE;
}

bool WzNetCamera::getParam(const QString &paramKey, QVariant &paramVal)
{
    QMutexLocker lock(m_paramMutex);
    if (paramKey == "imageInfo") {
        if (m_imageInfoList.count() == 0)
            return false;
        paramVal = m_imageInfoList.first();
        m_imageSize.setWidth(m_imageInfoList.first()["imageWidth"].toInt());
        m_imageSize.setHeight(m_imageInfoList.first()["imageHeight"].toInt());
        m_imageInfoList.removeFirst();
        return true;
    }
    if (!m_cameraParameters.contains(paramKey))
        return false;
    paramVal = m_cameraParameters[paramKey];
    return true;
}

void WzNetCamera::processImage()
{

}

void WzNetCamera::handleTimerFired()
{
    if (m_isPreviewEnabled && m_isPreviewUpdated) {
        emit previewImageUpdated();
        m_isPreviewUpdated = false;
    } else if (m_isCaptureFinished) {
        emit captureFinished();
        m_isCaptureFinished = false;
    }
}

void WzNetCamera::getCameraInfoTimer()
{
    run();
}

void WzNetCamera::readImageFromServer()
{
    m_tcpStream.startTransaction();
    m_tcpStream >> m_tcpData;
    if (!m_tcpStream.commitTransaction()) {
#ifdef TCP_DEBUG
        qDebug() << "wait of tcp data";
#endif
        return;
    }
#ifdef TCP_DEBUG
    qDebug() << "tcp data received, m_tcpData.length:" << m_tcpData.length();
    qDebug() << "\tmain thread:" << QApplication::instance()->thread()->currentThreadId()
             << ",current thread:" << thread()->currentThreadId();
#endif

    if (m_isPreviewEnabled) {
        {
            QMutexLocker lock(&m_mutexImageData);
            if (nullptr == m_previewImageData) {
                m_previewImageData = new uchar[m_tcpData.length()];
            } else if (m_previewImageDataSize < m_tcpData.length()) {
                delete [] m_previewImageData;
                m_previewImageData = new uchar[m_tcpData.length()];
            }
            m_previewImageDataSize = m_tcpData.length();
            memcpy(m_previewImageData, m_tcpData.data(), m_previewImageDataSize);
        }

        emit previewImageUpdated();

    }/* else if (m_downloadFile != "") {
        if (m_downloadFile == m_imageInfo["imageFile"].toString()) {
            m_downloadFile = "";
            QString imagePath = mkdirImagePath(m_imageInfo["captureDate"].toDate());
            QFileInfo remoteFileInfo(m_imageInfo["imageFile"].toString());
            QFileInfo localFileInfo(imagePath, remoteFileInfo.fileName());
            qDebug() << "imageFile, save to: " << localFileInfo.absoluteFilePath();
            QFile f(localFileInfo.absoluteFilePath());
            f.open(QFile::WriteOnly);
            f.write(m_tcpData);
            f.close();
            m_imageInfo["imageFile"] = localFileInfo.absoluteFilePath();
            setParam("imageInfo", m_imageInfo);
            m_latestImageFile = localFileInfo.absoluteFilePath();
            getFileFromServer(m_imageInfo["imageThumbFile"].toString());

        } else if (m_downloadFile == m_imageInfo["imageThumbFile"].toString()) {

            m_downloadFile = "";
            QString fileName = WzImageService::getThumbFileName(m_imageInfo["imageFile"].toString());
            qDebug() << "imageThumbFile, save to: " << fileName;
            QFile f(fileName);
            f.open(QFile::WriteOnly);
            f.write(m_tcpData);
            f.close();
            m_imageInfo["imageThumbFile"] = fileName;
            setParam("imageInfo", m_imageInfo);
            m_latestThumbFile = fileName;

            if (m_imageInfo["imageWhiteFile"].toString() != "") {
                getFileFromServer(m_imageInfo["imageWhiteFile"].toString());
            } else {
                m_waitImageTimer.stop();
                m_waitImageElapsedTimer.restart();
                emit cameraState(WzCameraState::CaptureFinished);
            }

        } else if (m_downloadFile == m_imageInfo["imageWhiteFile"].toString()) {

            m_downloadFile = "";
            QString imagePath = mkdirImagePath(m_imageInfo["captureDate"].toDate());
            QFileInfo remoteFileInfo(m_imageInfo["imageWhiteFile"].toString());
            QFileInfo localFileInfo(imagePath, remoteFileInfo.fileName());
            qDebug() << "imageThumbFile, save to: " << localFileInfo.absoluteFilePath();
            QFile f(localFileInfo.absoluteFilePath());
            f.open(QFile::WriteOnly);
            f.write(m_tcpData);
            f.close();
            m_imageInfo["imageWhiteFile"] = localFileInfo.absoluteFilePath();
            setParam("imageInfo", m_imageInfo);

            m_waitImageTimer.stop();
            m_waitImageElapsedTimer.restart();
            emit cameraState(WzCameraState::CaptureFinished);
        }
    }*/
}

void WzNetCamera::imageSocketError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "imageSocketError:" << socketError;
    if (QAbstractSocket::RemoteHostClosedError == socketError) {
        connectImageServer();
    }
}

void WzNetCamera::controlWsStateChanged(QAbstractSocket::SocketState state)
{
    m_ctlWsState = state;
    if (state == QAbstractSocket::ConnectedState) {
        getCameraInfoTimer();
        m_pGetCameraInfoTimer->start();
    }
    qDebug() << "cameraControlStateChanged:" << state;
}

void WzNetCamera::controlWsError(QAbstractSocket::SocketError error)
{
    qDebug() << "cameraControlError:" << error;
    if (error == QAbstractSocket::RemoteHostClosedError) {
        disconnect();
    }
}

void WzNetCamera::controlWsTextMessage(const QString &message)
{
    //qDebug() << "WzNetCamera::textMessage, " << message;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull())
        return;

    QJsonValue action = doc["action"];
    if (action.isUndefined()) {}
    else if (action.toString() == "getCameraInfo") {
        m_currentFrame = doc["m_currentFrame"].toInt();
        m_isPreviewEnabled = doc["m_isPreviewEnabled"].toBool();
        m_isCaptureFinished = doc["m_isCaptureFinished"].toInt();
        m_capturedCount = doc["m_capturedCount"].toInt();
        m_exposureMilliseconds = doc["m_exposureMilliseconds"].toInt();
        m_leftExposureMilliseconds = doc["m_leftExposureMilliseconds"].toInt();
        m_temperature = doc["m_temperature"].toDouble();
        m_isConnected = doc["m_cameraConnected"].toBool();
        m_frameCount = doc["m_captureCount"].toInt();

        // 此处使用状态列表, 如果只是获取服务端最新的相机状态, 可能因为网络延迟的缘故导致更早的状态丢失
        // 而使用状态列表的话, 只要客户端不取出这些状态, 服务端会一直维持这个列表, 直到客户端全部取出
        auto stateList = doc["m_cameraState"].toArray();

        /*
        qDebug() << "stateList:\n" << stateList;
        */

        // 客户端断开后重新连上, 服务器端依然是已连接状态且不会发送新的状态过来, 所以如果没有任何状态
        // 且相机是已连接则需要手动发一个相机已连接的状态到上层业务逻辑
        if (stateList.count() == 0) {
            // 2021-02-26 15:15:41 如果在服务端曝光时客户端异常退出, 客户端再连接后不会通过 stateList 收到新状态,
            // 所以在这里需要判断服务端的“当前状态”，如果是正在曝光或者其他状态而且客户端的状态与此不同则需要向上层发送此状态
            if (m_isConnected && m_cameraState != doc["m_cameraState2"].toInt()) {
                m_cameraState = static_cast<WzCameraState::CameraState>(doc["m_cameraState2"].toInt());
                emit this->cameraState(m_cameraState);
            }
        } else {
            auto iter = stateList.constBegin();
            while (iter != stateList.constEnd()) {
                auto map = iter->toVariant().toMap();
                auto newState = static_cast<WzCameraState::CameraState>(map["cameraState"].toInt());
                if (newState == WzCameraState::CaptureFinished) {
                    m_imageInfoList << map["imageInfo"].toMap();
                    getFileFromServer(m_imageInfoList.last());
                }
                if (m_cameraState != newState) {
                    m_cameraState = newState;
                    emit this->cameraState(m_cameraState);
                }                
                iter++;
            }
        }

    }

    m_latestReceivedServerMsgTime = QDateTime::currentDateTime();
}

void WzNetCamera::fileDownloadFinished(const int &id)
{
    qDebug() << "NetCamera::fileDownloadFinished, id:" << id;
    auto downloader = m_downloaders[id];
    if (downloader) {
        //for (int i = 0; i < downloader->localFiles().count(); i++)
        //    emit imageDownloaded(downloader->localFiles().at(i));
        // 只触发主图即可
        imageDownloaded(downloader->localFiles().first());
        m_downloaders.remove(id);
        downloader->deleteLater();
    }
}

void WzNetCamera::fileDownloadError(const int &id)
{
    auto downloader = m_downloaders[id];
    if (downloader) {
        m_downloaders.remove(id);
        downloader->deleteLater();
    }
}

int WzNetCamera::sendJson(const QJsonObject &object)
{
    QJsonDocument doc = QJsonDocument(object);
    QString json = doc.toJson();
    return m_pCtlWs->sendTextMessage(json);
}

void WzNetCamera::getFileFromServer(QVariantMap &imageInfo)
{
    qDebug() << "NetCamera::getFileFromServer";

    QStringList remoteFiles, localFiles;

    remoteFiles << imageInfo["imageFile"].toString();
    QFileInfo remoteFileInfo(remoteFiles.last());
    QString imagePath = mkdirImagePath(imageInfo["captureDate"].toDate());
    QFileInfo localFileInfo(QDir(imagePath), remoteFileInfo.fileName());
    imageInfo["imageFile"] = localFileInfo.absoluteFilePath();
    m_latestImageFile = localFileInfo.absoluteFilePath();
    localFiles << m_latestImageFile;

    remoteFiles << imageInfo["imageThumbFile"].toString();
    QString m_latestThumbFile = WzImageService::getThumbFileName(localFiles.first());
    imageInfo["imageThumbFile"] = m_latestThumbFile;
    localFiles << m_latestThumbFile;

    QString imageWhiteFile = imageInfo["imageWhiteFile"].toString();
    if (imageWhiteFile != "") {
        remoteFiles << imageWhiteFile;

        QDir markerPath(imagePath);
        markerPath.mkdir(kMarkerFolder);
        markerPath.cd(kMarkerFolder);

        QFileInfo remoteFileInfo(imageWhiteFile);
        QFileInfo localFileInfo(markerPath, remoteFileInfo.fileName());

        imageInfo["imageWhiteFile"] = localFileInfo.absoluteFilePath();
        localFiles << localFileInfo.absoluteFilePath();
    }

    QVariant serverAddress, serverFilePort;
    getParam("serverAddress", serverAddress);
    getParam("serverFilePort", serverFilePort);

    auto d = new WzFileDownloader;
    d->setServerAddress(serverAddress.toString());
    d->setServerPort(serverFilePort.toInt());
    QObject::connect(d, &WzFileDownloader::finished, this, &WzNetCamera::fileDownloadFinished);
    auto id = d->downloadFile(remoteFiles, localFiles);
    m_downloaders[id] = d;
}

void WzNetCamera::connectImageServer()
{
    QVariant serverAddress, serverImagePort;
    getParam("serverAddress", serverAddress);
    getParam("serverImagePort", serverImagePort);
    m_pImageTs->abort();
    m_pImageTs->disconnectFromHost();
    m_pImageTs->connectToHost(serverAddress.toString(), serverImagePort.toInt());
}

#ifdef delete
void WzNetCamera::waitImageTimer()
{
    // timeout
    if (m_waitImageElapsedTimer.hasExpired(1000 * 10)) {
        m_waitImageTimer.stop();
        m_waitImageElapsedTimer.restart();
        qWarning() << "downloading image timeout";
        emit cameraState(WzCameraState::Error);
        return;
    }    
}
#endif
