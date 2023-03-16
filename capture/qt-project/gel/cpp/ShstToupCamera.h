#ifndef SHSTTOUPCAMERA_H
#define SHSTTOUPCAMERA_H

#include <QObject>
#include <QtGlobal>
#include "../WzCamera.h"
#include "../ShstCameraInstance.h"
#include "../ShstToupCameraHandle.h"

//#include "KSJApi.h"
#include "toupcam.h"

//#define MAX_DEVICE 64

//struct KSJ_DEVICEINFO_TEMP {
//    int                 nIndex;
//    KSJ_DEVICETYPE		DeviceType;
//    int					nSerials;
//    WORD				wFirmwareVersion;
//    WORD                wFpgaVersion;
//};

class ShstToupCameraInstance : public ShstCameraInstance
{
    Q_OBJECT
public:
    explicit ShstToupCameraInstance(QObject *parent = nullptr);

    QString getName() const override;
    QString getDisplayName() const override;
    QString getModel() const override;

    void setModel(const QString &newModel);

    operator QString() const;
    void setName(const QString &newName);

    void setDisplayName(const QString &newDisplayName);

    const ToupcamDeviceV2 &toupcamDevice() const;
    void setToupcamDevice(const ToupcamDeviceV2 &newToupcamDevice);

private:
    QString m_name;
    QString m_displayName;
    QString m_model;
    ToupcamDeviceV2 m_toupcamDevice;
};

class ShstToupCamera : public WzAbstractCamera
{
    Q_OBJECT
public:
    explicit ShstToupCamera(QObject *parent = nullptr);
    ~ShstToupCamera() override;

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

    static QList<ShstCameraInstance*> s_instances; // s = static
    static int count();
    static QList<ShstCameraInstance*>* instances();
protected:
    void processImage() override;

private slots:
    void handleTimerFired();

private:
    ShstToupCameraHandle m_handle;
    int m_imageBitCount = 16;
    int m_bitsPerSample = 2;
    uint8_t* m_singleFrame = nullptr;
    uint16_t* m_grayAccumulateBuffer = nullptr;
    uint8_t* m_previewBuffer = nullptr;
    uint32_t m_previewBufferSize = 0;
    uint8_t* m_toupcamImageBuffer = nullptr;
    uint32_t m_toupcamImageBufferSize = 0;
    int32_t m_isConnected;
    int32_t m_run;
    WzCameraState::CameraState m_cameraState;

    int m_previewBinning = 1;
    int32_t m_isPreviewEnabled;
    int32_t m_isPreviewUpdated;
    int32_t m_isCapture;
    int32_t m_isCaptureMode = false;
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
    //KSJ_DEVICEINFO_TEMP m_deviceInfo[MAX_DEVICE];
    int m_deviceCurSel = 0;
    int m_maxExposure = 0;
    int m_minExposure = 0;
    ShstToupCameraInstance* m_activeCameraInstance = nullptr;

    bool openCamera();
    bool openCamera(ShstCameraInstance* instance);
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
private:
    // toupcam callback
    static void __stdcall eventCallBack(unsigned nEvent, void* pCallbackCtx);
    void handleImageCallback();
    void handleExpCallback();
};

#endif // SHSTTOUPCAMERA_H
