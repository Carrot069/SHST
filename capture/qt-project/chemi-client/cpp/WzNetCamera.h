#ifndef WZNETCAMERA_H
#define WZNETCAMERA_H

#include <QObject>
#include <QWebSocket>
#include <QDataStream>
#include <QMutexLocker>

#include "WzCamera.h"
#include "WzFileDownloader.h"

class WzNetCamera : public WzAbstractCamera
{
    Q_OBJECT
public:
    const QString CONTROL_URI = "/camera/control";

    explicit WzNetCamera(QObject *parent=nullptr);
    ~WzNetCamera() override;

    void connect() override;
    void disconnect() override;

    void run() override;

    int setPreviewEnabled(bool enabled) override;
    void resetPreview() override;
    QSize* getImageSize() override;
    int getImageBytes() override;
    int getImage(void* imageData) override;
    bool connected() override;
    int capture(const int exposureMilliseconds) override;
    int capture(const int* exposureMilliseconds, const int count) override;
    void abortCapture() override;
    int getExposureMs() override;
    int getLeftExposureMs() override;
    int getCurrentFrame() override;
    int getCapturedCount() override;
    QString getLatestImageFile() override;
    QString getLatestThumbFile() override;
    double getTemperature() override;
    int getMarkerImage(uint16_t **buffer) override;
    bool getParam(const QString& paramKey, QVariant& paramVal) override;

signals:
    void imageDownloaded(const QString &fileName);

protected:
    void processImage() override;

private slots:
    void handleTimerFired();

    void getCameraInfoTimer();

    void readImageFromServer();
    void imageSocketError(QAbstractSocket::SocketError socketError);

    void controlWsStateChanged(QAbstractSocket::SocketState state);
    void controlWsError(QAbstractSocket::SocketError error);
    void controlWsTextMessage(const QString &message);

    void fileDownloadFinished(const int &id);
    void fileDownloadError(const int &id);

private:
    QMap<int, WzFileDownloader*> m_downloaders;
    QList<QVariantMap> m_imageInfoList;
    QWebSocket *m_pCtlWs = nullptr; // Control WebSocket
    QTcpSocket* m_pImageTs = nullptr; // TcpSocket
    QDataStream m_tcpStream;
    QAbstractSocket::SocketState m_ctlWsState;

    int32_t m_isConnected = 0;
    int32_t m_run = 0;
    WzCameraState::CameraState m_cameraState = WzCameraState::None;

    int32_t m_isPreviewEnabled;
    int32_t m_isPreviewUpdated;
    int32_t m_isCapture;
    int32_t m_isCaptureFinished;
    bool m_isAbortCapture = false;
    int m_exposureMilliseconds = 0;
    int m_leftExposureMilliseconds = 0; // 当前拍摄的剩余曝光时间
    QString m_latestImageFile;          // 最后一次拍摄产生的图片的完整文件名
    QString m_latestThumbFile;          // 最后一次拍摄产生的缩略图的完整文件名
    double m_temperature = 0;

    QMutex m_mutexImageData;

    QDateTime m_latestReceivedServerMsgTime; // 最后收到服务器消息的时间
    QTimer* m_pGetCameraInfoTimer = nullptr;
    QSize m_imageSize;
    QByteArray m_tcpData;
    uint16_t* m_imageData = nullptr;
    uchar* m_previewImageData = nullptr; // 8位预览数据, 从16位转换的
    int m_previewImageDataSize = 0;
    void* m_pImageData = nullptr;
    uchar m_gray16to8Table[65536];

    int sendJson(const QJsonObject &object);
    void getFileFromServer(QVariantMap &imageInfo); // 准备废弃
    void connectImageServer(); // 建立预览图像传输连接
    //void waitImageTimer();

    // test code
    QByteArray m_testImage;
    // test code
};

#endif // WZNETCAMERA_H
