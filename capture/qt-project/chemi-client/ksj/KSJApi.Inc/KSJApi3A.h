#ifndef __KSJ_API_3A_H__
#define __KSJ_API_3A_H__

// #pragma message("Include KSJApi3A.h") 

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

	// 设置自动曝光的区域，这个区域是相对预览图像的左上角像素位置
	KSJ_API  int __stdcall KSJ_AESetRegion(int nChannel, int nX, int nY, int nW, int nH);
	// Get AE Region (according to preview image left top coner ) and Show Status.
	KSJ_API  int __stdcall KSJ_AEGetRegion(int nChannel, int *pnX, int *pnY, int *pnW, int *pnH, bool *pbShow);
	// 设置自动曝光的区域，这个区域是相对预览图像的左上角像素位置,是否显示区域
	KSJ_API  int __stdcall KSJ_AEShowRegion(int nChannel, bool bShow);

	KSJ_API  int __stdcall KSJ_AESetPeakAveRatio(int nChannel, float fRatio);
	KSJ_API  int __stdcall KSJ_AEGetPeakAveRatio(int nChannel, float *pfRatio);

	// Set Exposure Time Range When Do Auto Exposure
	KSJ_API  int __stdcall KSJ_AESetExposureTimeRange(int nChannel, float fMinExpMs,   float fMaxExpMs);
	KSJ_API  int __stdcall KSJ_AEGetExposureTimeRange(int nChannel, float *pfMinExpMs, float *pfMaxExpMs);

	// AE Status return to callback
	enum KSJ_AE_STATUS { KSJ_AE_ONCE_SUCCESS = 0, KSJ_AE_ONCE_FAIL_MAX_COUNT, KSJ_AE_ONCE_FAIL_WAVE, KSJ_AE_ONCE_FAIL_OVER_RANGE, KSJ_AE_DOING, KSJ_AE_CONTINUE_SUCCESS, KSJ_AE_CONTINUE_FAIL_WAVE, KSJ_AE_CONTINUE_FAIL_OVER_RANGE };
	// Auto Exposure Ex Callback, Call when AE Doing, Success, Fail.
	typedef void(__stdcall *KSJ_AECALLBACKEX)( KSJ_AE_STATUS Status, int nExpsoureTimeMs, void *lpContext);
	KSJ_API  int __stdcall KSJ_AESetCallbackEx(int nChannel, KSJ_AECALLBACKEX pfAECallbackEx, void *lpContext);

	KSJ_API  int __stdcall KSJ_AESetMaxCount(int nChannel, int  nMaxCount);
	KSJ_API  int __stdcall KSJ_AESetTarget  (int nChannel, int  nTarget  );
	KSJ_API  int __stdcall KSJ_AEStartEx    (int nChannel, bool bStart   );
	KSJ_API  int __stdcall KSJ_AEGetStatusEx(int nChannel, bool *pbStart, int *pnMaxCount, int *pnTarget);

	// Auto Exposure Callback, Call when AE Finish.
	typedef void(__stdcall *KSJ_AECALLBACK)(bool bSuccess, int nResult, void *lpContext);
	// 定义自动曝光操作结束后的回调函数，当自动曝光结束时，KSJ开发包会自动调用AECallback用户指定的回调函数
	KSJ_API  int __stdcall KSJ_AESetCallback(int nChannel, KSJ_AECALLBACK pfAECallback, void *lpContext);
	// 开始或终止自动曝光调节
	KSJ_API  int __stdcall KSJ_AEStart(int nChannel, bool bStart, int nMaxCount, int nTarget);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 设置自动白平衡区域，这个区域是相对预览图像的左上角像素位置
	KSJ_API  int __stdcall KSJ_AWBSetRegion(int nChannel, int nX, int nY, int nW, int nH);
	// 设置自动白平衡区域，这个区域是相对预览图像的左上角像素位置,是否显示区域
	KSJ_API  int __stdcall KSJ_AWBShowRegion(int nChannel, bool bShow);
	KSJ_API  int __stdcall KSJ_AWBGetRegion(int nChannel, int *pnX, int *pnY, int *pnW, int *pnH, bool *pbShow);

	// 自动白平衡回调函数
	typedef void(__stdcall *KSJ_AWBCALLBACK)(bool bSuccess, int nValueR, int nValueG, int nValueB, void *lpContext);
	// 定义自动白平衡（操作硬件）操作结束后的回调函数，当自动曝光结束时，KSJ开发包会自动调用AECallback用户指定的回调函数
	KSJ_API  int __stdcall KSJ_AWBSetCallback(int nChannel, KSJ_AWBCALLBACK fpAWBCallback, void *lpContext);
	// 开始或终止自动白平衡调节
	KSJ_API  int __stdcall KSJ_AWBStart(int nChannel, bool bStart, int nMaxCount, int nRTarget, int nGTarget, int nBTarget);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 设置聚焦评估区域
	KSJ_API  int __stdcall KSJ_AFSetRegion(int nChannel, int nX, int nY, int nW, int nH);
	KSJ_API  int __stdcall KSJ_AFShowRegion(int nChannel, bool bShow);
	KSJ_API  int __stdcall KSJ_AFGetRegion(int nChannel, int *pnX, int *pnY, int *pnW, int *pnH, bool *pbShow);
	// 聚焦评估回调函数, nValue为返回的评估值
	typedef void(__stdcall *KSJ_AFCALLBACK)(int nValue, void *lpContext);
	// 设置聚焦评估回调
	KSJ_API  int __stdcall KSJ_AFSetCallback(int nChannel, KSJ_AFCALLBACK fpAFCallback, void *lpContext);
	// 开始或终止聚焦评估
	KSJ_API  int __stdcall KSJ_AFStart(int nChannel, bool bStart);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 硬件自动曝光
	KSJ_API  int __stdcall KSJ_AECSetEnable ( int nChannel, bool bEnable  );
	KSJ_API  int __stdcall KSJ_AECGetEnable ( int nChannel, bool *pbEnable  );

	KSJ_API  int __stdcall KSJ_AECSetRange ( int nChannel, unsigned short usMin,  unsigned short usMax  );
	KSJ_API  int __stdcall KSJ_AECGetRange ( int nChannel, unsigned short *pusMin, unsigned short *pusMax );
	// 得到自动曝光调节的当前曝光值
	KSJ_API  int __stdcall KSJ_AECGetCur ( int nChannel, unsigned short *usCur  );

	// 硬件自动增益
	KSJ_API  int __stdcall KSJ_AGCSetEnable ( int nChannel, bool bEnable  );
	KSJ_API  int __stdcall KSJ_AGCGetEnable ( int nChannel, bool *pbEnable  );

	KSJ_API  int __stdcall KSJ_AGCSetRange ( int nChannel, unsigned short usMin,  unsigned short usMax  );
	KSJ_API  int __stdcall KSJ_AGCGetRange ( int nChannel, unsigned short *pusMin, unsigned short *pusMax );

	// 得到自动增益调节的当前增益值
	KSJ_API  int __stdcall KSJ_AGCGetCur ( int nChannel, unsigned short *pusCur  );

#ifdef __cplusplus
}
#endif

#endif