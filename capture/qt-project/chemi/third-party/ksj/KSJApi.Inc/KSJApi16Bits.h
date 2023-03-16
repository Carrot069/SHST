#ifndef __KSJ_API_16BITS_H__
#define __KSJ_API_16BITS_H__

// #pragma message("Include KSJApi16Bits.h") 

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

	// Get AD Resolution. Valid bits in 16 bits.
	KSJ_API  int __stdcall KSJ_GetADCResolution(int nChannel, int *pnADCResolution);
	// Set 16bits Transfer, Image Data Bytes Number doubled.
	KSJ_API  int __stdcall KSJ_SetData16Bits(int nChannel, bool bEnable);
	// Get If 16bits Transfer
	KSJ_API  int __stdcall KSJ_GetData16Bits(int nChannel, bool *bEnable);
	// Get Capture Image Information. nBitsPerSample = 8 when not 16bits. = 16 when 16 bits.
	// Allocate Capture buffer according to these information.
	KSJ_API  int __stdcall KSJ_CaptureGetSizeExEx(int nChannel, int *pnWidth, int *pnHeight, int *pnBitCount, int *nBitsPerSample);
	// Save to Tiff Format.
	// When Not 16Bits, nBitPerSample = 8; When 16 Bits, nBitPerSample = 16
	// when BitCount = 8, nSamplesPerPixel = 1; When Bitcount = 24, nSamplesPerPixel = 3; When Bitcount = 32, nSamplesPerPixel = 4
	KSJ_API  int __stdcall KSJ_HelperSaveToTiff(unsigned char *pData, int nWidth, int nHeight, int nBitPerSample, int nSamplesPerPixel, const TCHAR *pszFileName);

#ifdef __cplusplus
}
#endif

#endif