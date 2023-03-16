#ifndef __KSJ_API_WATER_MARK__H__
#define __KSJ_API_WATER_MARK__H__

// #pragma message("Include KSJApiWaterMark.h") 

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

// 相关功能查询KSJ_SUPPORT_WATERMAK
enum KSJ_WATERMARKMODE { KSJ_WMM_TIMESTAMP, KSJ_WMM_DISTINGUISH_EXTERNAL_TRIGGER, KSJ_WMM_ANALYSIS_EXTERNAL_TRIGGER };

KSJ_API  int __stdcall KSJ_WaterMarkSetMode(int nChannel, KSJ_WATERMARKMODE WaterMarkMode);
KSJ_API  int __stdcall KSJ_WaterMarkGetMode(int nChannel, KSJ_WATERMARKMODE *pWaterMarkMode);

KSJ_API  int __stdcall KSJ_WaterMarkSetEnable(int nChannel, bool bEnable);
KSJ_API  int __stdcall KSJ_WaterMarkGetEnable(int nChannel, bool *pbEnable);

KSJ_API  int __stdcall KSJ_WaterMarkGetInfo(int nChannel, unsigned short *pwId, unsigned long *pdwTimestampLowPart, unsigned long *pdwTimestampHighPart);

#ifdef __cplusplus
}
#endif

#endif
