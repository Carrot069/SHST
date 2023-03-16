#ifndef __KSJ_API_STREAM_H__
#define __KSJ_API_STREAM_H__

// #pragma message("Include KSJApiBase.h") 

#ifdef KSJAPI_EXPORTS
#define KSJ_API __declspec(dllexport)
#elif defined KSJAPI_STATIC
#define KSJ_API
#else
#define KSJ_API __declspec(dllimport)
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifdef __cplusplus
extern "C"{
#endif

	KSJ_API  int __stdcall KSJ_IsSupportStreamMode(int nIndex, bool* pbSupport);

	KSJ_API  int __stdcall KSJ_StreamIsStart(int nIndex, bool* pbStart);

	KSJ_API  int __stdcall KSJ_StreamStart(int nIndex);
	KSJ_API  int __stdcall KSJ_StreamStop(int nIndex);

	KSJ_API  int __stdcall KSJ_StreamReadRawData(int nIndex, unsigned char *pRawData);
	KSJ_API  int __stdcall KSJ_StreamReadRgbData(int nIndex, unsigned char *pRgbData);

	KSJ_API  int __stdcall KSJ_StreamReadRawDataEx(int nIndex, unsigned char *pRawData, unsigned int* pDataSize);
	KSJ_API  int __stdcall KSJ_StreamReadRgbDataEx(int nIndex, unsigned char *pRgbData, unsigned int* pDataSize);

#ifdef __cplusplus
}
#endif

#endif
