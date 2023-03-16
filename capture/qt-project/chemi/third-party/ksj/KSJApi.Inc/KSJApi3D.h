#ifndef __KSJ_API_3D_H__
#define __KSJ_API_3D_H__


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
	
	KSJ_API  int __stdcall KSJ_3DInit(int nChannel);

	KSJ_API  int __stdcall KSJ_3DSetThresh(int nChannel, int nThresh);
	KSJ_API  int __stdcall KSJ_3DGetThresh(int nChannel, int* pnThresh);
	KSJ_API  int __stdcall KSJ_3DSetAutoThresh(int nChannel, bool bAuto);
	KSJ_API  int __stdcall KSJ_3DGetAutoThresh(int nChannel, bool* pbAuto);

	KSJ_API  int __stdcall KSJ_3DSetRoi(int nChannel, int nColStart, int nRowStart, int nColSize, int nRowSize);
	KSJ_API  int __stdcall KSJ_3DGetRoi(int nChannel, int* pnColStart, int* pnRowStart, int* pnColSize, int* pnRowSize);

	KSJ_API  int __stdcall KSJ_3DGetRange(int nChannel, float* pfZRange, float* pfXRangeNear, float* pfXRangeFar);

	KSJ_API  int __stdcall KSJ_3DCapturePointCloudData(int nChannel, unsigned char* pImageData, float* pfZ, float* pfX);

	KSJ_API  int __stdcall KSJ_3DClcPointCloudData(int nChannel, unsigned char* pImageData, float* pfZ, float* pfX);

#ifdef __cplusplus
}
#endif

#endif
