#ifndef WZCAPTURE_SERVICE_H
#define WZCAPTURE_SERVICE_H

#ifdef ATIK
#include "WzAtikCamera.h"
#endif

#include <memory>
#include <fstream>

#include <QObject>
#include <QVariant>
#include <QDebug>
#include <QJsonObject>

#include "WzDatabaseService.h"
#include "WzUtils.h"
#include "WzLiveImageView.h"
#include "WzGlobalEnum.h"
#include "WzCamera.h"
#include <WzRenderThread.h>
#ifdef PVCAM
#include "WzPvCamera.h"
#endif
#ifdef GEL_CAPTURE
#include "WzKsjCamera.h"
#endif
#ifdef HARDLOCK
#include "RY3_API.h"
#endif
#include "WzAutoFocus.h"

class WzCaptureService : public QObject, public WzCameraCallback
{
    Q_OBJECT
public:

    enum CaptureState {Init,     // 正在进行初始化，开始曝光之前的所有动作都列入该状态
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
    Q_INVOKABLE bool enableCameraPreview(bool enabled, int binning, bool isSlow = false);
    Q_INVOKABLE bool resetPreview(); // 修改相机参数后重启预览, 比如binning和曝光时间
    Q_INVOKABLE bool capture(QVariantMap params);
    Q_INVOKABLE bool captureMulti(QVariantMap params);
    Q_INVOKABLE void abortCapture();
    Q_INVOKABLE int getCameraTemperature();
    Q_INVOKABLE void setCameraParam(const QString& paramName, const QVariant paramVal);
    Q_INVOKABLE QVariant getCameraParam(const QString& paramName);
    Q_INVOKABLE bool hasMarkerImage(const int threshold);
    Q_INVOKABLE bool getMarkerImage();
    Q_INVOKABLE void clearMarkerImage();
    Q_INVOKABLE QString saveMarkerImage(const QDateTime &dateTime);
    Q_INVOKABLE void startAutoFocus();
    Q_INVOKABLE void stopAutoFocus();
    Q_INVOKABLE QString getPreviewImageDiff(const int diffType = 0);

    Q_PROPERTY(int previewExposureMs MEMBER m_previewExposureMs WRITE setPreviewExposureMs) // 预览的曝光时间
    Q_PROPERTY(int exposurePercent MEMBER m_exposurePercent)              // 当前曝光时间百分比
    Q_PROPERTY(int captureCount MEMBER m_captureCount)                    // 拍摄次数
    Q_PROPERTY(int captureIndex MEMBER m_captureIndex)                    // 当前拍摄的次数-1
    Q_PROPERTY(int capturedCount MEMBER m_capturedCount)                  // 已经拍完的数量
    Q_PROPERTY(WzCameraState::CameraState cameraState READ cameraState NOTIFY cameraStateChanged)
    Q_PROPERTY(QString leftExposureTime MEMBER m_leftExposureTime)        // 本次拍摄剩余曝光时间
    Q_PROPERTY(QString elapsedExposureTime MEMBER m_elapsedExposureTime)  // 本次拍摄已经曝光的时间
    Q_PROPERTY(QString latestImageFile MEMBER m_latestImageFile)          // 最新完成拍摄的图片文件名
    Q_PROPERTY(QString latestThumbFile MEMBER m_latestThumbFile)          // 最新完成拍摄的缩略图文件名
    Q_PROPERTY(int cameraCount READ getCameraCount CONSTANT)
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)
    Q_PROPERTY(bool isAutoExposure READ getAutoExposure WRITE setAutoExposure NOTIFY autoExposureChanged)
    Q_PROPERTY(int autoExposureMs READ getAutoExposureMs CONSTANT)

    bool cameraSN(const QString sn);

signals:
    void cameraStateChanged(const WzCameraState::CameraState& state);
    void captureState(const WzCaptureService::CaptureState& state);

    void autoFocusFar(const int step);
    void autoFocusNear(const int step);
    void autoFocusStop();
    void autoFocusFinished();
    void autoFocusNewFrame();

    void imagePathChanged();
    void autoExposureChanged();

private slots:
    void previewImageUpdated();
    void captureTimer();
    void cameraStateUpdated(const WzCameraState::CameraState& state);

    // auto focus
    void autoFocusGetImage();
    void autoFocusLog(const QString &msg);

private:
    WzAutoFocus m_autoFocus;

    WzAbstractCamera* m_camera;
    uchar* m_previewImageData;
    QMutex m_previewImageDataLock;
    QSize* m_previewImageSize;
    uint64_t m_previewCount = 0;
    QTimer* m_captureTimer;
    QVariantMap m_captureParams;

    // 如果正在预览，点击拍摄后此数据中保存的是预览的最后一张图片，保存下来用于叠加显示暗场图
    uint16_t* m_markerTiff16bit = nullptr;
    QMutex m_markerTiff16bitLock;
    QSize m_markerImageSize;
    //bool m_autoGotMarker = false;

    int m_previewExposureMs;             // 预览的曝光时间
    int m_exposurePercent;               // 当前曝光时间百分比
    int m_captureCount;                  // 拍摄次数
    int m_captureIndex;                  // 当前拍摄的次数-1
    int m_capturedCount;                 // 已经拍完的数量
    int64_t m_captureId = 0;             // 本次拍摄的ID，多帧拍摄时每一帧的ID相同，这个ID取自1970年以来的秒数
    QString m_leftExposureTime;          // 本次拍摄剩余曝光时间
    QString m_elapsedExposureTime;       // 本次拍摄已经曝光的时间
    QString m_latestImageFile;
    QString m_latestThumbFile;
    bool m_cameraConnected = false;
    QString m_imagePath = "";
    bool m_isAutoExposure = false;

    WzCameraState::CameraState m_cameraState;

    QList<QString> m_cameraSNList;

    int getCameraCount();
    void setPreviewExposureMs(int exposureMs);
    void readCameraSNList(); // 读取允许的使用的相机序列号列表文件
    QString imagePath();
    void setImagePath(const QString imagePath);
    WzCameraState::CameraState cameraState();
    bool getAutoExposure();
    void setAutoExposure(bool autoExposure);
    int getAutoExposureMs();
};

#endif // WZCAPTURE_SERVICE_H
