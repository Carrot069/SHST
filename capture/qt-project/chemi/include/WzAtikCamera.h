#ifndef WZATIKCAMERA_H
#define WZATIKCAMERA_H

#include <fstream>
#include <exception>

#include <QtGlobal>
#include <QMutex>
#include <QElapsedTimer>

#include "atik/AtikCameras.h"

#include "WzCamera.h"
#include "WzImageFilter.h"

class WzAtikCamera : public WzAbstractCamera
{
    Q_OBJECT
public:
    explicit WzAtikCamera(QObject *parent=nullptr);
    ~WzAtikCamera() override;

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
    int getMarkerImage(uint16_t **buffer) override;

protected:
    void processImage() override;

private slots:
    void handleTimerFired();

private:
    uint16_t* m_singleFrame = nullptr;
    uint16_t* m_grayAccumulateBuffer = nullptr;
    uint16_t* m_previewBuffer = nullptr;
    uint32_t m_previewBufferSize = 0;
    int32_t m_isConnected;
    int32_t m_run;

    int m_previewBinning = 1;
    int32_t m_isPreviewEnabled;
    int32_t m_isPreviewUpdated;
    bool m_resetPreview = false;
    bool m_needResetBin = false;
    int32_t m_isCapture;
    int32_t m_isCaptureFinished;
    int32_t m_isAbortCapture;
    bool m_isAbortGetImageData = false;
    int* m_exposureMilliseconds = nullptr;
    int m_leftExposureMilliseconds = 0; // 当前拍摄的剩余曝光时间
#ifdef EXPOSURE_TIME_DOUBLE
    int m_exposureTimeMultiplier = 2;
    int m_realExposureMs = 0;
    int m_additionExposureMs = 0;
#endif
    QString m_latestImageFile;          // 最后一次拍摄产生的图片的完整文件名
    QString m_latestThumbFile;          // 最后一次拍摄产生的缩略图的完整文件名

    QMutex m_mutexImageData;

    QTimer* m_timer;
    WzCameraThread* m_thread;
    QSize* m_imageSize;
    uint32_t m_imageBytes;
    uint8_t* m_previewImage8bit = nullptr; // 8位预览数据, 从16位转换的
    uint16_t* m_previewImage16bit = nullptr; // 16位原始相机数据, 叠加显示化学发光图使用, 如果先开始预览再拍摄则将其自动保存
    QMutex m_previewImage16bitMutex;
    int m_previewImage16bitLength = 0;
    void* m_pImageData = nullptr;
    uint8_t* m_gray16to8Table = nullptr;

    bool m_isSdkInitialized = false;
    bool m_isCameraOpen = false;
    ArtemisHandle handle = nullptr;
    int16_t m_numberOfCameras = 0;
    uint16_t m_SensorResX;
    uint16_t m_SensorResY;
    double m_temperature = 0;
    int16_t m_minSetTemperature = 0, m_maxSetTemperature = 0;
    bool m_getTemperature = true;
    char m_camSerNum[100];
    int m_nTempSensor = 0;

    bool initSDK();
    bool uninitSdk();
    bool openCamera();
    bool closeCamera();
    bool getLatestFrame(uint16_t *frameAddress, const int timeout);
    bool setupPreview();
    bool startPreview();
    bool continuePreview(const int exposureTime);
    bool stopPreview();

    bool singleCaptureSetup();
    bool singleCaptureStart(uint32_t exposureMs);
    bool singleCaptureAbort();
    bool singleCaptureFinished();

    void updateTemperature();
    bool setTemperature(double temperature);

    int autoExposure(int step, int destBin);
};

#endif // WZATIKCAMERA_H
