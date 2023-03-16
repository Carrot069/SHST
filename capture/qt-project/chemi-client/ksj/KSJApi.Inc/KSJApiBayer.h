#ifndef __KSJ_API_BAYER_H__
#define __KSJ_API_BAYER_H__

// #pragma message("Include KSJApiBayer.h") 

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

	// Bayer Mode
	enum KSJ_BAYERMODE 
	{
		KSJ_BGGR_BGR24 = 0, 
		KSJ_GRBG_BGR24, 
		KSJ_RGGB_BGR24,
		KSJ_GBRG_BGR24,
		KSJ_BGGR_BGR24_FLIP, 
		KSJ_GRBG_BGR24_FLIP, 
		KSJ_RGGB_BGR24_FLIP, 
		KSJ_GBRG_BGR24_FLIP,
		KSJ_BGGR_BGR32, 
		KSJ_GRBG_BGR32,
		KSJ_RGGB_BGR32,
		KSJ_GBRG_BGR32, 
		KSJ_BGGR_BGR32_FLIP, 
		KSJ_GRBG_BGR32_FLIP, 
		KSJ_RGGB_BGR32_FLIP, 
		KSJ_GBRG_BGR32_FLIP,
		KSJ_BGGR_GRAY8, 
		KSJ_GRBG_GRAY8, 
		KSJ_RGGB_GRAY8, 
		KSJ_GBRG_GRAY8, 
		KSJ_BGGR_GRAY8_FLIP, 
		KSJ_GRBG_GRAY8_FLIP,
		KSJ_RGGB_GRAY8_FLIP, 
		KSJ_GBRG_GRAY8_FLIP
	};

	// 得到当前相机的默认Bayer模式
	KSJ_API   int __stdcall KSJ_BayerGetDefaultMode(int nChannel, KSJ_BAYERMODE *pBayerMode);
	// 设置Bayer模式，同时影响采集和预览，这个函数除特殊需要调用，否则用户不需调用此功能。
	KSJ_API   int __stdcall KSJ_BayerSetMode(int nChannel, KSJ_BAYERMODE BayerMode);
	// 获得当前Bayer模式
	KSJ_API   int __stdcall KSJ_BayerGetMode(int nChannel, KSJ_BAYERMODE *pBayerMode);

	// Bayer Filter Algorithm 
	enum KSJ_FILTERMODE
	{
		KSJ_NEARESTNEIGHBOR,
		KSJ_BILINEAR,
		KSJ_SMOOTHHUE,
		KSJ_EDGESENSING,
		KSJ_LAPLACIAN,
		KSJ_FASTBILINEAR
	};

	// 设置Bayer Filter模式，同时影响采集和预览，这个函数除特殊需要调用，否则用户不需调用此功能。
	KSJ_API   int __stdcall KSJ_FilterSetMode(int nChannel, KSJ_FILTERMODE FilterMode);
	// 获得当前Bayer模式
	KSJ_API   int __stdcall KSJ_FilterGetMode(int nChannel, KSJ_FILTERMODE *pFilterMode);

#ifdef __cplusplus
}
#endif

#endif
