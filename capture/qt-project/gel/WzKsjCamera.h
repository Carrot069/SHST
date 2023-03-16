#ifndef WZKSJCAMERA_H
#define WZKSJCAMERA_H

#ifdef WIN32
#include <QtGlobal>

#include "WzCamera.h"
#include "KSJApi.h"

#define MAX_DEVICE 64

struct KSJ_DEVICEINFO {
    int                 nIndex;
    KSJ_DEVICETYPE		DeviceType;
    int					nSerials;
    WORD				wFirmwareVersion;
    WORD                wFpgaVersion;
};

class WzKsjCamera : public WzAbstractCamera
{
    Q_OBJECT
public:
    explicit WzKsjCamera(QObject *parent=nullptr);
    ~WzKsjCamera() override;

    void connect() override;
    void disconnect() override;

    void run() override;

    int setPreviewEnabled(bool enabled) override;
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
    bool isRgbImage() const override;

    void newFrame(unsigned char *pImageData, int nWidth, int nHeight, int nBitCount);

protected:
    void processImage() override;

private slots:
    void handleTimerFired();

private:
    int m_imageBitCount = 16;
    int m_bitsPerSample = 2;
    uint8_t* m_singleFrame = nullptr;
    uint16_t* m_grayAccumulateBuffer = nullptr;
    uint8_t* m_previewBuffer = nullptr;
    uint32_t m_previewBufferSize = 0;
    int32_t m_isConnected;
    int32_t m_run;
    WzCameraState::CameraState m_cameraState;

    int m_previewBinning = 1;
    int32_t m_isPreviewEnabled;
    int32_t m_isPreviewUpdated;
    int32_t m_isCapture;
    int32_t m_isCaptureFinished;
    int32_t m_isAbortCapture;
    int* m_exposureMilliseconds = nullptr;
    int32_t m_capturedCount = 0;
    int32_t m_frameCount = 0;           // 需要拍摄的数量
    int32_t m_currentFrame = 0;         // 当前正在拍摄帧索引
    int m_leftExposureMilliseconds = 0; // 当前拍摄的剩余曝光时间
    QString m_latestImageFile;          // 最后一次拍摄产生的图片的完整文件名
    QString m_latestThumbFile;          // 最后一次拍摄产生的缩略图的完整文件名

    QMutex m_mutexImageData;

    QTimer* m_timer;
    QTimer* m_getTemperatureTimer;
    WzCameraThread* m_thread;
    QSize* m_imageSize;
    uint32_t m_imageBytes;
    uchar* m_previewImage8bit = nullptr; // 8位预览数据, 从16位转换的
    uint16* m_previewImage16bit = nullptr; // 16位原始相机数据, 叠加显示化学发光图使用, 如果先开始预览再拍摄则将其自动保存
    void* m_pImageData = nullptr;
    uchar m_gray16to8Table[65536];

    QMutex m_EofMutex;
    QWaitCondition m_EofCond;
    bool m_EofFlag = false;

    bool m_isCamInitialized = false;
    bool m_isCameraOpen = false;
    int16 m_numberOfCameras = 0;
    QString m_latestErrorMsg;
    int m_SensorResX;
    int m_SensorResY;
    KSJ_DEVICEINFO m_deviceInfo[MAX_DEVICE];
    int m_deviceCurSel = 0;
    int m_maxExposure = 0;
    int m_minExposure = 0;

    bool initKSJ();
    bool uninitKSJCAM();
    bool openCamera();
    bool closeCamera();
    bool getLatestFrame(uint8_t **frameAddress);
    bool setupPreview();
    bool startPreview();
    bool stopPreview();
    bool checkRet(const int retCode, const QString msg);

    bool singleCaptureSetup();
    bool singleCaptureStart(uint32_t exposureMs);
    bool singleCaptureAbort();
    bool waitEof(uint milliseconds);
    bool singleCaptureFinished();
    bool copyPreviewToSingleFrame();

    QString getSettingIniFileName();
    int readGammaFromIni();
    void readRGBGainFromIni(int &redGain, int &greenGain, int &blueGain);
    void checkExposureTime(uint32_t *exposureMs);
};
#endif

#endif // WZKSJCAMERA_H
