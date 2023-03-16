#ifndef __KSJ_API_IMAGE_H__
#define __KSJ_API_IMAGE_H__

// #pragma message("Include KSJApiImage.h") 

#ifdef KSJAPI_EXPORTS
    #define KSJ_API __declspec(dllexport)
#elif defined KSJAPI_STATIC
	#define KSJ_API
#else
    #define KSJ_API __declspec(dllimport)
#endif



#ifdef __cplusplus
extern "C"{
#endif

    // 坏点校正低阈值和高阈值设置（硬件不支持此设置，由软件处理时有效）
    KSJ_API  int __stdcall KSJ_BadPixelCorrectionSetThreshold(int nChannel, unsigned char btLow, unsigned char btHigh);
    KSJ_API  int __stdcall KSJ_BadPixelCorrectionGetThreshold(int nChannel, unsigned char *pbtLow, unsigned char *pbtHigh);

    enum KSJ_THRESHOLD 
    { 
	    KSJ_THRESHOLD_LOW, 
	    KSJ_THRESHOLD_HIGH 
    };
    // 坏点校正设置及阈值设置，低阈值会过滤更多的坏点
    KSJ_API  int __stdcall KSJ_BadPixelCorrectionSet(int nChannel, bool bEnable, KSJ_THRESHOLD Threshold);
    KSJ_API  int __stdcall KSJ_BadPixelCorrectionGet(int nChannel, bool *pbEnable, KSJ_THRESHOLD *pThreshold);

    // Same to Parameter KSJ_CONTRAST（-128~128）， Software Preccess, may influence capture efficiency or preivew frame rate.
    KSJ_API  int __stdcall KSJ_ProcessContrastGetRange(int nChannel, int *pnMinContrast, int *pnMaxContrast);
    KSJ_API  int __stdcall KSJ_ProcessContrastSet(int nChannel, int nContrast);
    KSJ_API  int __stdcall KSJ_ProcessContrastGet(int nChannel, int *pnContrast);

	// Same to Parameter KSJ_BRIGHTNESS（-128~128）, Software Preccess, may influence capture efficiency or preivew frame rate.
    KSJ_API  int __stdcall KSJ_ProcessBrightnessGetRange(int nChannel, int *pnMinBrightness, int *pnMaxBrightness);
    KSJ_API  int __stdcall KSJ_ProcessBrightnessSet(int nChannel, int nBrightness);
    KSJ_API  int __stdcall KSJ_ProcessBrightnessGet(int nChannel, int *pnBrightness);

	// Same to Parameter KSJ_SATURATION（-128~128）, Software Preccess, may influence capture efficiency or preivew frame rate.
    KSJ_API  int __stdcall KSJ_ProcessSaturationGetRange(int nChannel, int *pnMinSaturation, int *pnMaxSaturation);
    KSJ_API  int __stdcall KSJ_ProcessSaturationSet(int nChannel, int nSaturation);
    KSJ_API  int __stdcall KSJ_ProcessSaturationGet(int nChannel, int *pnStaturation);

	// 设置Gamma值，范围 -9 - +9
    KSJ_API  int __stdcall KSJ_GammaGetRange(int nChannel, int *pnMinGamma, int *pnMaxGamm);
    KSJ_API  int __stdcall KSJ_GammaSetValue( int nChannel, int nValue );
    KSJ_API  int __stdcall KSJ_GammaGetValue(int nChannel, int *pnValue);

    typedef enum
    {
	    KSJ_ALL_HUES,
	    KSJ_RED_HUES,
	    KSJ_YELLOW_HUES,
	    KSJ_GREEN_HUES,
	    KSJ_CYAN_HUES,
	    KSJ_BLUE_HUES,
	    KSJ_MAGENTA_HUES
    } KSJHueRange;

    // nHue: -180 ~ 180, nSaturation: -100 ~ 100, nLightness: -100 ~ 100
    KSJ_API  int __stdcall KSJ_HueSaturationSet(int nChannel, KSJHueRange HueRange, int nHue, int nSaturation, int nLightness);
    KSJ_API  int __stdcall KSJ_HueSaturationGet(int nChannel, KSJHueRange HueRange, int *pnHue, int *pnSaturation, int *pnLightness);
    KSJ_API  int __stdcall KSJ_HueSaturationSetEnable(int nChannel, bool bEnable);
	KSJ_API  int __stdcall KSJ_HueSaturationGetEnable(int nChannel, bool *pbEnable);

#ifdef __cplusplus
}
#endif

#endif


