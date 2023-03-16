#ifndef SHCAMERA_H
#define SHCAMERA_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMap>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QSize>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include <QDir>
#include <QFileInfo>
#include <QImage>

#ifdef WINNT
#include <windows.h>
#include "tiff.h"
#include "tiffio.h"
#endif

#ifdef MAC
#include "tiff.h"
#include "tiffio.h"
#endif

#include "WzImageService.h"
#include "WzUtils.h"
#include "WzGlobalConst.h"
#include "WzImageService.h"

const int32_t DEMO_CAMERA_WIDTH = 2688;
const int32_t DEMO_CAMERA_HEIGHT = 2200;

const uint32_t ERROR_NONE = 0;  // 无错误
const uint32_t ERROR_PARAM = 1; // 参数错误
const uint32_t ERROR_NULLPTR = 2; // 缓冲区指针无效
const uint32_t ERROR_DATA_LEN_0 = 3; // 数据长度为0

class WzCameraCallback {
public:
    virtual bool cameraSN(const QString sn) = 0;
};

class WzCameraThread;

class WzCameraState : public QObject
{
    Q_OBJECT
public:
    WzCameraState(QObject *parent=nullptr): QObject(parent) {}
    enum CameraState{None,
                     Connecting,
                     Connected,
                     PreviewStarting,
                     PreviewStarted,
                     PreviewStopping,
                     PreviewStopped,
                     AutoExposure,
                     AutoExposureFinished,
                     CaptureInit,
                     CaptureAborting, // 正在取消
                     CaptureAborted,  // 已经取消拍摄
                     Exposure,
                     Image,           // 曝光完成后对图片的处理, 比如降噪、保存等
                     CaptureFinished, // 拍摄完成
                     Disconnecting,
                     Disconnected,
                     CameraNotFound,  // 没找到任何相机(同一种类的)
                     Error            // 出现错误, 具体错误信息会输出到日志
                    };
    Q_ENUM(CameraState)
};

class WzAbstractCamera : public QObject
{
    Q_OBJECT
public:
    WzAbstractCamera(QObject *parent=nullptr);
    virtual ~WzAbstractCamera() override;

    virtual void connect() = 0;
    virtual void disconnect() = 0;

    virtual void run() = 0;

    void setParam(const QString& paramKey, const QVariant& paramVal);
    bool getParam(const QString& paramKey, QVariant& paramVal);
    virtual QSize* getImageSize() = 0; // 图像的宽高
    virtual int getImageBytes() = 0;   // 图像的字节数
    virtual int getImage(void* imageData) = 0;
    virtual int setPreviewEnabled(bool enabled) = 0;
    virtual void resetPreview() = 0;
    virtual int capture(const int exposureMilliseconds) = 0;
    virtual int capture(const int* exposureMilliseconds, const int count) = 0;
    virtual void abortCapture() = 0;
    virtual int getExposureMs() = 0;        // 当前正在拍摄的曝光时间, 因为有多帧拍摄功能, 所以这个数值是变化的
    virtual int getLeftExposureMs() = 0;
    virtual int getCurrentFrame() = 0;
    virtual int getCapturedCount() = 0;
    virtual QString getLatestImageFile() = 0;
    virtual QString getLatestThumbFile() = 0;
    virtual double getTemperature() = 0;
    virtual int getMarkerImage(uint16_t** buffer) = 0;
    void setCameraCallback(WzCameraCallback* callback);

    static uint16_t* changeImageSize(uint16_t* imageBuffer, int& imageWidth,
        int& imageHeight, const qreal& zoom);
    static bool getMaxAndAvgGray(uint16_t* grayBuffer, int countOfPixel,
                                 uint16_t* grayAvg, uint16_t* maxGray, int64_t* allGray);


protected:
    int32_t m_frameCount = 0;           // 需要拍摄的数量
    int32_t m_currentFrame = 0;         // 当前正在拍摄帧索引
    int m_capturedCount = 0;
    WzCameraState::CameraState m_cameraState;

    QMap<QString, QVariant> m_cameraParameters;
    WzCameraCallback* m_cameraCallback = nullptr;
    virtual void processImage();                    // 对原始的图片进行处理，比如降噪和保存到硬盘
    QString mkdirImagePath();                       // 建立图片存储目录, 返回目录
    QString getTiffFileName(const QString& path, const QDateTime &dateTime);   // 获取按照一定规则生成的文件名, 包含完整路径
    void newCameraState(const WzCameraState::CameraState &newCameraState);
    // 在子类中 emit 父类的 signal 会有编译警告
    void emitPreviewImageUpdated();
    void emitCaptureFinished();

signals:
    void previewImageUpdated();
    void captureFinished();
    void cameraState(const WzCameraState::CameraState& cameraState);

private:
    QMutex* m_paramMutex;
};

class WzCameraThread : public QThread
{
    Q_OBJECT
public:
    explicit WzCameraThread(QObject *parent) ;
private:
    WzAbstractCamera* m_camera;
    void run() override;
};

class WzTestCamera : public WzAbstractCamera
{
    Q_OBJECT
public:
    explicit WzTestCamera(QObject *parent=nullptr);
    ~WzTestCamera() override;

    void connect() override;
    void disconnect() override;

    void run() override;

    int setPreviewEnabled(bool enabled) override;
    void resetPreview() override;
    QSize* getImageSize() override;
    int getImageBytes() override;
    int getImage(void* imageData) override;
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
    int getMarkerImage(uint16_t **imageData) override;

protected:
    void processImage() override;

private slots:
    void handleTimerFired();

private:
    int32_t m_isConnected;
    int32_t m_run;

    int32_t m_isPreviewEnabled;
    int32_t m_isPreviewUpdated;
    int32_t m_isCapture;
    int32_t m_isCaptureFinished;
    int32_t m_isAbortCapture;
    int* m_exposureMilliseconds = nullptr;
    int32_t m_isGrayAccumulate = 0;     // 是否累计信号
    int m_leftExposureMilliseconds = 0; // 当前拍摄的剩余曝光时间
    QString m_latestImageFile;          // 最后一次拍摄产生的图片的完整文件名
    QString m_latestThumbFile;          // 最后一次拍摄产生的缩略图的完整文件名

    QMutex m_mutexImageData;

    QTimer* m_timer;
    WzCameraThread* m_thread;
    QSize* m_imageSize;
    int m_imageBytes;
    uint16_t* m_imageData;
    uchar* m_previewImageData; // 8位预览数据, 从16位转换的
    void* m_pImageData;
    uchar m_gray16to8Table[65536];
};


#endif // SHCAMERA_H
