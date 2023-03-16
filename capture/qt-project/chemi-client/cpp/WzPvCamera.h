#ifndef WZPVCAMERA_H
#define WZPVCAMERA_H

#ifdef WIN32
#include <QtGlobal>
#include "WzCamera.h"
#include "WzImageFilter.h"

// PVCAM
#include <master.h>
#include <pvcam.h>

/*
 * Common data types
 */

// TODO Move to WzPvCamera.h
// Name-Value Pair type - an item in enumeration type
typedef struct NVP
{
    int32 value;
    std::string name;
}
NVP;
// Name-Value Pair Container type - an enumeration type
typedef std::vector<NVP> NVPC;

// Each camera has one or more ports, this structure holds information with port
// descriptions. Each camera port has one or more speeds (readout frequencies).
// On most EM cameras there are two ports - one EM and one non-EM port with one
// or two speeds per port.
// On non-EM camera there is usually one port only with multiple speeds.
typedef struct READOUT_OPTION
{
    NVP port;
    int16 speedIndex;
    float readoutFrequency;
    int16 bitDepth;
    std::vector<int16> gains;
}
READOUT_OPTION;


class WzPvCamera : public WzAbstractCamera
{
    Q_OBJECT
public:
    explicit WzPvCamera(QObject *parent=nullptr);
    ~WzPvCamera() override;

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

    void newFrame();

protected:
    void processImage() override;

private slots:
    void handleTimerFired();

private:
    uns16* m_singleFrame = nullptr;
    uns16* m_grayAccumulateBuffer = nullptr;
    uns16* m_previewBuffer = nullptr;
    uns32 m_previewBufferSize = 0;
    int32_t m_isConnected;
    int32_t m_run;
    WzCameraState::CameraState m_cameraState;

    int m_previewBinning = 1;
    int32_t m_isPreviewEnabled;
    int32_t m_isPreviewUpdated;
    bool m_resetPreview = false;
    int32_t m_isCapture;
    int32_t m_isCaptureFinished;
    int32_t m_isAbortCapture;
    int* m_exposureMilliseconds = nullptr;
    int m_leftExposureMilliseconds = 0; // 当前拍摄的剩余曝光时间
    QString m_latestImageFile;          // 最后一次拍摄产生的图片的完整文件名
    QString m_latestThumbFile;          // 最后一次拍摄产生的缩略图的完整文件名

    QMutex m_mutexImageData;

    QTimer* m_timer;
    QTimer* m_getTemperatureTimer;
    WzCameraThread* m_thread;
    QSize* m_imageSize;
    uns32 m_imageBytes;
    uchar* m_previewImage8bit = nullptr; // 8位预览数据, 从16位转换的
    uint16* m_previewImage16bit = nullptr; // 16位原始相机数据, 叠加显示化学发光图使用, 如果先开始预览再拍摄则将其自动保存
    int m_previewImage16bitLength = 0;
    void* m_pImageData = nullptr;
    uchar m_gray16to8Table[65536];

    QMutex m_EofMutex;
    QWaitCondition m_EofCond;
    bool m_EofFlag;

    rs_bool m_isPvcamInitialized = false;
    rs_bool m_isCameraOpen = false;
    int16 m_hCam = -1;
    int16 m_numberOfCameras = 0;
    QString m_latestErrorMsg;
    rs_bool m_isFrameTransfer = false;
    rs_bool m_IsSmartStreaming;
    std::vector<READOUT_OPTION> m_SpeedTable;
    uns16 m_SensorResX;
    uns16 m_SensorResY;
    double m_temperature = 0;
    int16 m_minSetTemperature = 0, m_maxSetTemperature = 0;
    bool m_getTemperature = true;
    char m_camSerNum[MAX_ALPHA_SER_NUM_LEN];

    bool initPVCAM();
    bool uninitPVCAM();
    bool openCamera();
    bool closeCamera();
    bool getLatestFrame(uns16 **frameAddress);
    bool setupPreview();
    bool startPreview();
    bool stopPreview();
    void printErrorMessage(int16 errorCode, const char *message);
    bool isParamAvailable(uns32 paramID, const char *paramName);
    bool readEnumeration(NVPC *nvpc, uns32 paramID, const char *paramName);
    bool setDefectivePixelCorrection(const bool &isEnabled);

    bool singleCaptureSetup();
    bool singleCaptureStart(uns32 exposureMs);
    bool singleCaptureAbort();
    bool waitEof(uint milliseconds);
    bool singleCaptureFinished();

    void updateTemperature();
    bool setTemperature(double temperature);

    int autoExposure(int step, int destBin);
};
#endif

#endif // WZPVCAMERA_H
