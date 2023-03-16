#ifndef __KSJ_API_CALLBACK_H__
#define __KSJ_API_CALLBACK_H__

// #pragma message("Include KSJApiCallback.h") 

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

     // Preview Callback
	typedef void(CALLBACK *KSJ_PREVIEWCALLBACK)(unsigned char *pImageData, int nWidth, int nHeight, int nBitCount, void *lpContext);
    KSJ_API  int __stdcall KSJ_PreviewSetCallback( int nChannel, KSJ_PREVIEWCALLBACK pfPreviewCallback, void *lpContext );

    typedef void(CALLBACK *KSJ_PREVIEWCALLBACKEX)(HDC  hDC, unsigned char *pImageData, int nWidth, int nHeight, int nBitCount, void *lpContext);
    KSJ_API  int __stdcall KSJ_PreviewSetCallbackEx( int nChannel, KSJ_PREVIEWCALLBACKEX pfPreviewCallbackEx, void *lpContext );


#ifdef __cplusplus
}
#endif

#endif