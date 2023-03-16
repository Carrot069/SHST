#ifndef __KSJ_API_CUSTOM_H__
#define __KSJ_API_CUSTOM_H__

// #pragma message("Include KSJApiCustom.h") 

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

	// 将pData(宽度为nWidth, 高度为nHeight, 比特位数为nBitCount)的图像数据缩放为宽度为nStretchWidh, 高度为nStretchHeight的数据并填充到用户分配的pStretchData缓冲区,比特位数不变
	KSJ_API  int __stdcall KSJ_HelperStretchBmp(unsigned char *pData, int nWidth, int nHeight, int nBitCount, unsigned char *pStretchData, int nStretchWidth, int nStretchHeight);
	// 根据图像的比特深度得到BMP图像的文件头,信息头,及颜色表总的字节数目
	KSJ_API  int __stdcall KSJ_HelperGetBmpFileHeaderSize(int nBitCount, int *pnTotalBytes);
	// 得到BMP图像的文件头和信息头
	KSJ_API  int __stdcall KSJ_HelperGetBmpFileHeader(int nWidth, int nHeight, int nBitCount, void *pHeader);
	// 采集一帧图像到系统剪贴板
	KSJ_API  int __stdcall KSJ_CaptureRgbDataToClipboard(int nChannel);

	// 设置FFC视场
	KSJ_API  int __stdcall KSJ_SetFlatFieldCorrectionFieldOfView(int nChannel, int nColStart, int nRowStart, int nColSize, int nRowSize, KSJ_ADDRESSMODE AmCol, KSJ_ADDRESSMODE AmRow, unsigned short wMultiFrameNum);

	// 获取FFC的视场，视场必须相同才可以进行校正。
	KSJ_API  int __stdcall KSJ_GetFlatFieldCorrectionFieldOfView(int nChannel, int *pnColStart, int *pnRowStart, int *pnColSize, int *pnRowSize, KSJ_ADDRESSMODE *pColAddressMode, KSJ_ADDRESSMODE *pRowAddressMode, unsigned short *pwMultiFrameNum);

	// 根据采集视场采集nDarkFrames张图像，平均后作为DarkFrame
	KSJ_API  int __stdcall KSJ_MakeDarkFrame(int nChannel, int nDarkFrames);
	// 根据采集视场采集nFlatFieldFrames张图像，平均后作为FlatFieldImage
	KSJ_API  int __stdcall KSJ_MakeFlatFieldFrame(int nChannel, int nFlatFieldFrames);

	// 预览时是否使能
	KSJ_API  int __stdcall KSJ_PreviewSetFlatFieldCorrection(int nChannel, bool bEnable);

	KSJ_API  int __stdcall KSJ_PreviewGetFlatFieldCorrection(int nChannel, bool *pbEnable);
	// 采集时是否使能
	KSJ_API  int __stdcall KSJ_CaptureSetFlatFieldCorrection(int nChannel, bool bEnable);

	KSJ_API  int __stdcall KSJ_CaptureGetFlatFieldCorrection(int nChannel, bool *pbEnable);

	KSJ_API  int __stdcall KSJ_SaveFlatFieldCorrection(int nChannel, TCHAR *pszFilePath);

	KSJ_API  int __stdcall KSJ_LoadFlatFieldCorrection(int nChannel, TCHAR *pszFilePath);


	// 预览时是否使能
	KSJ_API  int __stdcall KSJ_PreviewSetCalibration(int nChannel, bool bEnable);

	KSJ_API  int __stdcall KSJ_PreviewGetCalibration(int nChannel, bool *pbEnable);
	// 采集时是否使能
	KSJ_API  int __stdcall KSJ_CaptureSetCalibration(int nChannel, bool bEnable);

	KSJ_API  int __stdcall KSJ_CaptureGetCalibration(int nChannel, bool *pbEnable);

	KSJ_API  int __stdcall KSJ_LoadCalibrationMapFile(int nChannel, const TCHAR *pszFilePath);

	enum KSJ_MAPMODE 
	{
		KSJ_MM_NEARESTNEIGHBOR,
		KSJ_MM_BILINEAR
	};

	KSJ_API  int __stdcall KSJ_SetCalibrationMapMode(int nChannel, KSJ_MAPMODE MapMode);
	KSJ_API  int __stdcall KSJ_GetCalibrationMapMode(int nChannel, KSJ_MAPMODE *pMapMode);

#ifdef __cplusplus
}
#endif

#endif
