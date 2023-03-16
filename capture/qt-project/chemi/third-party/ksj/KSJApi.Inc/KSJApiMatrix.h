#ifndef __KSJ_API_MATRIX_H__
#define __KSJ_API_MATRIX_H__

// #pragma message("Include KSJApiMatrix.h") 

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

	// SWB - Software White Balance; HWB - Hardware White Balance
	enum KSJ_WB_MODE
	{
		KSJ_WB_DISABLE,
		KSJ_SWB_PRESETTINGS,
		KSJ_SWB_AUTO_ONCE,
		KSJ_SWB_AUTO_CONITNUOUS,
		KSJ_SWB_MANUAL,
		KSJ_HWB_PRESETTINGS,
		KSJ_HWB_AUTO_ONCE,
		KSJ_HWB_AUTO_CONITNUOUS,
		KSJ_HWB_MANUAL
	};

	// Set White Balance Matrix Mode
	KSJ_API  int __stdcall KSJ_WhiteBalanceSet(int nChannel, KSJ_WB_MODE WbMode);
	// Get White Balance Matrix Mode
	KSJ_API  int __stdcall KSJ_WhiteBalanceGet(int nChannel, KSJ_WB_MODE *pWbMode);

	// Set nPhi when KSJ_SWB_AUTO_ONCE or KSJ_SWB_AUTO_CONITNUOUS
	KSJ_API  int __stdcall KSJ_WhiteBalanceAutoSet(int nChannel, int nPhi);
	KSJ_API  int __stdcall KSJ_WhiteBalanceAutoGet(int nChannel, int *pnPhi);


	// Set when KSJ_SWB_MANUAL or KSJ_HWB_MANUAL
	// Rw = fMatrix[0] * R, Gw = fMatrix[1] * G, Bw = fMatrix[1] * B
	// Range 0 - 5
	KSJ_API  int __stdcall KSJ_WhiteBalanceMatrixSet(int nChannel, float fMatrix[3]);
	// You can get current White Balance Matrix on Image
	KSJ_API  int __stdcall KSJ_WhiteBalanceMatrixGet(int nChannel, float fMatrix[3]);

	// Call Callback Function When KSJ_AWB_AUTO_ONCE or KSJ_AWB_AUTO_CONITNUOUS Done. Finall Matrix will Return.
	typedef void(__stdcall *KSJ_WB_CALLBACK)(float fMatrix[3], void *lpContext);
	KSJ_API  int __stdcall KSJ_WhiteBalanceAutoSetCallBack(int nChannel, KSJ_WB_CALLBACK    pfCallBack, void *lpContext);      // The nPhi decide auto calculate Matrix.

	// Call Callback Function When KSJ_AWB_AUTO_ONCE or KSJ_AWB_AUTO_CONITNUOUS Done. Finall Matrix will Return.
	typedef void(__stdcall *KSJ_WB_CALLBACK_EX)(float fMatrixR, float fMatrixG, float fMatrixB, void *lpContext);
	KSJ_API  int __stdcall KSJ_WhiteBalanceAutoSetCallBackEx(int nChannel, KSJ_WB_CALLBACK_EX pfCallBackEx, void *lpContext);

	enum KSJ_COLOR_TEMPRATURE 
	{ 
		KSJ_CT_5000K, 
		KSJ_CT_6500K, 
		KSJ_CT_2800K 
	};
	// Set Preseting White Balance Matrix according to Color Temprature when in KSJ_SWB_PRESETTINGS or KSJ_HWB_PRESETTINGS.
	KSJ_API  int __stdcall KSJ_WhiteBalancePresettingSet(int nChannel, KSJ_COLOR_TEMPRATURE WbPresetting);
	KSJ_API  int __stdcall KSJ_WhiteBalancePresettingGet(int nChannel, KSJ_COLOR_TEMPRATURE *pWbPresetting);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SCCM - Software Color Correction Matrix; HCCM - Hardware Color Correction Matrix
	enum KSJ_CCM_MODE
	{
		KSJ_CCM_DISABLE,
		KSJ_SCCM_PRESETTINGS,
		KSJ_SCCM_MANUAL,
		KSJ_HCCM_PRESETTINGS,
		KSJ_HCCM_MANUAL
	};

	KSJ_API  int __stdcall KSJ_ColorCorrectionSet(int nChannel, KSJ_CCM_MODE CcmMode);
	KSJ_API  int __stdcall KSJ_ColorCorrectionGet(int nChannel, KSJ_CCM_MODE *pCcmMode);


	// Set Color Correction Matrix when in KSJ_SCCM_MANUAL or KSJ_HCCM_MANUAL
	// Rc = fMatrix[0][0] * Rw + fMatrix[0][1] * Gw + fMatrix[0][2] * Bw
	// Range -5 - 5
	KSJ_API  int __stdcall KSJ_ColorCorrectionMatrixSet(int nChannel, float fMatrix[3][3]);
	// Get current Color Correction Matrix on Image
	KSJ_API  int __stdcall KSJ_ColorCorrectionMatrixGet(int nChannel, float fMatrix[3][3]);


	// Set Preseting Color Correction Matrix according to Color Temprature when KSJ_SCCM_PRESETTINGS or KSJ_HCCM_PRESETTINGS.
	KSJ_API  int __stdcall KSJ_ColorCorrectionPresettingSet(int nChannel, KSJ_COLOR_TEMPRATURE CcmPresetting);
	KSJ_API  int __stdcall KSJ_ColorCorrectionPresettingGet(int nChannel, KSJ_COLOR_TEMPRATURE *pCcmPresetting);



#ifdef __cplusplus
}
#endif

#endif