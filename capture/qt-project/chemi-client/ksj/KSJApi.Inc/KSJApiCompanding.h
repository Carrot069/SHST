#ifndef __KSJ_API_COMPANDING_H__
#define __KSJ_API_COMPANDING_H__

// #pragma message("Include KSJApiCompanding.h") 

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

    // 参考“凯视佳工业相机API函数说明书（数据压扩篇）”
	// Related API KSJ_GetADCResolution to Get Valid bits of 16bits
	// Please KSJ_QueryFunction ( KSJ_SUPPORT_LUT_DOWNLOAD ) to Get if support download LUT
	// KtbFile Must Created by KSJShow. Download 16 to 8 Look Up Data to Camera, it will lost when power off.
	KSJ_API  int __stdcall KSJ_LutFileDownload(int nChannel, const TCHAR *pszKtbFile);
    // Download 16 to 8 Look Up Data to Camera, it will lost when power off.
    KSJ_API  int __stdcall KSJ_LutDataDownload( int nChannel, unsigned char *pData, int nBytes   );
    // Enable LUT
    KSJ_API  int __stdcall KSJ_LutSetEnable( int nChannel, bool bEnable   );
    // Get If LUT Enable
    KSJ_API  int __stdcall KSJ_LutGetEnable( int nChannel, bool *pbEnable );

	// Please KSJ_QueryFunction ( KSJ_SUPPORT_GAMMA_LUT_DOWNLOAD ) to Get if support download Gamma LUT
	// KtbFile Must Created by KSJShow. Download 16 to 16 Gamma Look Up Data to Camera, it will lost when power off.
    KSJ_API  int __stdcall KSJ_GammaLutFileDownload( int nChannel, const TCHAR *pszKtbFile );
	// Download Gamma Look Up Data to Camera, it will lost when power off.
    KSJ_API  int __stdcall KSJ_GammaLutDataDownload( int nChannel, unsigned short *pData, int nWords );
    // Enable Gamma LUT
    KSJ_API  int __stdcall KSJ_GammaLutSetEnable( int nChannel, bool bEnable   );
    // Get If Gamma LUT Enable
    KSJ_API  int __stdcall KSJ_GammaLutGetEnable( int nChannel, bool *pbEnable );

    // 灵敏度模式设置（使用截取8位方式）
    enum KSJ_SENSITIVITYMODE 
    {
	    KSJ_LOW = 0, 
	    KSJ_MID, 
	    KSJ_HIGH, 
	    KSJ_HIGHER, 
	    KSJ_HIGHEST 
    };
    // Related API KSJ_GetADCResolution
    KSJ_API  int __stdcall KSJ_SensitivitySetMode(int nChannel, KSJ_SENSITIVITYMODE SensitivityMode);
    KSJ_API  int __stdcall KSJ_SensitivityGetMode(int nChannel, KSJ_SENSITIVITYMODE *pSensitivityMode);

#ifdef __cplusplus
}
#endif

#endif