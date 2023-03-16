#ifndef WZCAPTURE_SERVICE_H
#define WZCAPTURE_SERVICE_H

#include <QObject>
#include <QVariant>
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

#include "WzDatabaseService.h"
#include "WzUtils.h"
#include "WzLiveImageView.h"
#include "WzGlobalEnum.h"
#include "WzCamera.h"
#include "WzNetCamera.h"
#include "WzUdpBroadcastReceiver.h"

class WzCaptureService : public QObject, public WzCameraCallback
{
    Q_OBJECT
public:

    enum CaptureState {None,
                       Init,     // 正在进行初始化，开始曝光之前的所有动作都列入该状态
                       AutoExposure, // 正在自动曝光
                       AutoExposureFinished, // 自动曝光完成
                       Exposure, // 正在曝光
                       Image,    // 曝光结束后所有动作都列入该状态
                       Finished, // 处理完图片且保存到硬盘, 所有动作都完成后进入该状态
                       Aborted   // 已取消
                      };
    Q_ENUM(CaptureState)

    explicit WzCaptureService(QObject *parent = nullptr);
    ~WzCaptureService();

    Q_INVOKABLE void connectCamera();
    Q_INVOKABLE void disconnectCamera();
    Q_INVOKABLE bool enableCameraPreview(bool enabled, int binning);
    Q_INVOKABLE bool resetPreview(); // 修改相机参数后重启预览, 比如binning和曝光时间
    Q_INVOKABLE bool capture(QVariantMap params);
    Q_INVOKABLE bool captureMulti(QVariantMap params);
    Q_INVOKABLE void abortCapture();
    Q_INVOKABLE int getCameraTemperature();
    Q_INVOKABLE void setCameraParam(const QString& paramName, const QVariant paramVal);
    Q_INVOKABLE bool getMarkerImage();
    Q_INVOKABLE void clearMarkerImage();
    Q_INVOKABLE void saveAdminSetting(const QString &params);
    Q_INVOKABLE void startAutoFocus(const QRect &focusRect);
    Q_INVOKABLE void stopAutoFocus();
    Q_INVOKABLE void openUdp(const int port);

    Q_PROPERTY(int previewExposureMs MEMBER m_previewExposureMs WRITE setPreviewExposureMs) // 预览的曝光时间
    Q_PROPERTY(int exposurePercent MEMBER m_exposurePercent)              // 当前曝光时间百分比
    Q_PROPERTY(int captureCount MEMBER m_captureCount)                    // 拍摄次数
    Q_PROPERTY(int captureIndex MEMBER m_captureIndex)                    // 当前拍摄的次数-1
    Q_PROPERTY(int capturedCount MEMBER m_capturedCount)                  // 已经拍完的数量
    Q_PROPERTY(CaptureState captureState MEMBER m_captureState)
    Q_PROPERTY(WzCameraState::CameraState cameraState READ cameraState NOTIFY cameraStateChanged)
    Q_PROPERTY(QString leftExposureTime MEMBER m_leftExposureTime)        // 本次拍摄剩余曝光时间
    Q_PROPERTY(QString elapsedExposureTime MEMBER m_elapsedExposureTime)  // 本次拍摄已经曝光的时间
    Q_PROPERTY(QString latestImageFile MEMBER m_latestImageFile)          // 最新完成拍摄的图片文件名
    Q_PROPERTY(QString latestThumbFile MEMBER m_latestThumbFile)          // 最新完成拍摄的缩略图文件名
    Q_PROPERTY(int cameraCount READ getCameraCount)
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath)
    Q_PROPERTY(QString serverAddress READ getServerAddress WRITE setServerAddress NOTIFY serverAddressChanged)
    Q_PROPERTY(int mcuPort MEMBER m_mcuPort CONSTANT)
    Q_PROPERTY(bool isAutoExposure READ getAutoExposure WRITE setAutoExposure)
    Q_PROPERTY(int autoExposureMs READ getAutoExposureMs)
    Q_PROPERTY(int connectFailedCount READ getConnectFailedCount)
    Q_PROPERTY(bool isAutoFocusing READ getIsAutoFocusing NOTIFY isAutoFocusingChanged)
    Q_PROPERTY(bool cameraConnected READ cameraConnected NOTIFY cameraConnectedChanged)

    bool cameraSN(const QString sn);

    bool getIsAutoFocusing() const;

signals:
    void connectServerTimeout(const int timeoutCount);
    void cameraStateChanged(const WzCameraState::CameraState& state);
    void captureStateChanged(const CaptureState& state);
    void previewImageRefreshed();
    void imageLoaded(const QString &fileName);
    void adminParamsReceived(const QJsonObject &adminParams);
    void serverAddressChanged(const QString &serverAddress);    
    void cameraConnectedChanged(const bool cameraConnected);
    void serverMessage(const QJsonObject &message);

private slots:
    void previewImageUpdated();
    void captureTimer();
    void cameraStateUpdated(const WzCameraState::CameraState& state);

    void controlWsStateChanged(QAbstractSocket::SocketState state);
    void controlWsError(QAbstractSocket::SocketError error);
    void controlWsTextMessage(const QString &message);

private:
    QTimer *m_pReconnectCameraTimer = nullptr; // 重连服务器
    void reconnectCameraTimer();
    int m_connectFailedCount = 0;
    int getConnectFailedCount() const;
    QWebSocket *m_pCtlWs = nullptr; // Control WebSocket
    QTcpSocket* m_pImageTs = nullptr; // TcpSocket
    QDataStream m_imageStream;
    int m_serverImagePort = 60051; // 服务器端传输实时预览图像的端口
    int m_serverControlPort = 60052; // 服务器端传输相机控制指令的端口
    int m_serverFilePort = 60054; // 服务器端传输(拍摄图片)文件的端口
    QAbstractSocket::SocketState m_CtlWsState;
    int sendJson(const QJsonObject &object);
    WzUdpBroadcastReceiver *m_udpBroadcastReceiver = nullptr;
    QString m_serverHostName = "";

    WzAbstractCamera* m_camera = nullptr;
    uchar* m_previewImageData = nullptr;
    //QSize* m_previewImageSize = nullptr;
    int m_previewImageBytesCount = 0;
    uint64_t m_previewCount = 0;
    QTimer* m_captureTimer = nullptr;
    QVariantMap m_captureParams;

    // 如果正在预览，点击拍摄后此数据中保存的是预览的最后一张图片，保存下来用于叠加显示暗场图
    uint16_t* m_markerTiff16bit = nullptr;
    QSize m_markerImageSize;

    int m_previewExposureMs;             // 预览的曝光时间
    int m_exposurePercent;               // 当前曝光时间百分比
    int m_captureCount;                  // 拍摄次数
    int m_captureIndex;                  // 当前拍摄的次数-1
    int m_capturedCount;                 // 已经拍完的数量
    CaptureState m_captureState;
    int64_t m_captureId = 0;             // 本次拍摄的ID，多帧拍摄时每一帧的ID相同，这个ID取自1970年以来的秒数
    QString m_leftExposureTime;          // 本次拍摄剩余曝光时间
    QString m_elapsedExposureTime;       // 本次拍摄已经曝光的时间
    QString m_latestImageFile;
    QString m_latestThumbFile;
    bool m_cameraConnected = false;
    QString m_imagePath = "";
    QString m_serverAddress = "";
    int m_mcuPort = 0;
    bool m_isAutoExposure = false;

    WzCameraState::CameraState m_cameraState;

    QList<QString> m_cameraSNList;

    int getCameraCount();
    void setPreviewExposureMs(int exposureMs);
    QString saveMarkerImage(const QDateTime &dateTime);
    void readCameraSNList(); // 读取允许的使用的相机序列号列表文件
    QString imagePath();
    void setImagePath(const QString imagePath);
    WzCameraState::CameraState cameraState();
    bool getAutoExposure();
    void setAutoExposure(bool autoExposure);
    int getAutoExposureMs();
    QString getServerAddress() const;
    void setServerAddress(const QString &serverAddress);
    bool cameraConnected();

    bool m_isAutoFocusing = false;
    QTimer m_getAutoFocusStateTimer;
signals:
    void isAutoFocusingChanged();

private slots:
    void getAutoFocusStateTimer();
    void serverHostNameChanged(const QString &hostName);
};

#endif // WZCAPTURE_SERVICE_H
