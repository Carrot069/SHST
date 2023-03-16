#ifndef __KSJ_API_RECORD_H__
#define __KSJ_API_RECORD_H__

// #pragma message("Include KSJApiRecord.h") 

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

	// Start or Stop Record. Must Start Preview, Record FOV is equal to Preivew FOV.
	KSJ_API  int __stdcall KSJ_RecordStart(int nChannel, bool bStart, float fRecordRate, const TCHAR *pFileName);
	// Start or Stop Record if Show Compossor Selection Dialog. Must Start Preview, Record FOV is equal to Preivew FOV.
	KSJ_API  int __stdcall KSJ_RecordStartEx(int nChannel, bool bStart, float fRecordRate, const TCHAR *pFileName, bool bShowCompossorSelection);

	// Start or Stop Record Use KSJ_RecordAddOneFrame to Add Frame Data.
	// Note: nWidth, nHeight, nBitCount must equal to KSJ_RecordAddOneFrame's parameter
	KSJ_API  int __stdcall KSJ_RecordStartExEx(int nChannel, bool bStart, float fRecordRate, int nWidth, int nHeight, int nBitCount,  const TCHAR *pFileName, bool bShowCompossorSelection);
	// Add one Frame after call KSJ_RecordStartExEx.
	KSJ_API  int __stdcall KSJ_RecordAddOneFrame(int nChannel, unsigned char *pFrameData, int nFrameNum, int nWidth, int nHeight, int nBitcount);
	// Get Record Status.
	KSJ_API  int __stdcall KSJ_RecordStatusGet(int nChannel, bool *bStart);

#ifdef __cplusplus
}
#endif

#endif