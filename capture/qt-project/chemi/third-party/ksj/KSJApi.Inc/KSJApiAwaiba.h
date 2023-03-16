#ifndef __KSJ_API_AWAIBA_H__
#define __KSJ_API_AWAIBA_H__

// #pragma message("Include KSJApiAwaiba.h") 

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
	KSJ_API  int __stdcall KSJ_AWAIBA_IsUsed( int nChannel, bool *bUsed );
	KSJ_API  int __stdcall KSJ_AWAIBA_GetSegmentNum(int nChannel, int *pnSegNum );

	KSJ_API  int __stdcall KSJ_AWAIBA_GetGainRange(int nChannel, int nSegIndex, int *pnMin, int *pnMax);
	KSJ_API  int __stdcall KSJ_AWAIBA_SetGain(int nChannel, int nSegIndex, int nGain);
	KSJ_API  int __stdcall KSJ_AWAIBA_GetGain(int nChannel, int nSegIndex, int *pnGain);

	KSJ_API  int __stdcall KSJ_AWAIBA_GetBlackLevelRange(int nChannel, int nSegIndex, int *pnMin, int *pnMax);
	KSJ_API  int __stdcall KSJ_AWAIBA_SetBlackLevel(int nChannel, int nSegIndex, int nBlackLevel);
	KSJ_API  int __stdcall KSJ_AWAIBA_GetBlackLevel (int nChannel, int nSegIndex, int *pnBlackLevel);
	KSJ_API  int __stdcall KSJ_AWAIBA_AutoBlackLevel(int nChannel, int nSegIndex);

	KSJ_API  int __stdcall KSJ_AWAIBA_SetSplitSegment(int nChannel, bool bSplit);
	KSJ_API  int __stdcall KSJ_AWAIBA_GetSplitSegment(int nChannel, bool *pbSplit);

	// 如下函数未实现
	/*
	KSJ_API  int __stdcall KSJ_AWAIBA_SetAntiBlooming(int nChannel, bool bEnable);
	KSJ_API  int __stdcall KSJ_AWAIBA_GetAntiBlooming(int nChannel, bool *bEnable);

	KSJ_API  int __stdcall KSJ_AWAIBA_SetAntiCorona(int nChannel, bool bEnable);
	KSJ_API  int __stdcall KSJ_AWAIBA_GetAntiCorona(int nChannel, bool *bEnable);

	enum KSJ_AWAIBA_ANALOGUE_GAIN {KSJ_AAG_X1, KSJ_AAG_X4};
	KSJ_API  int __stdcall KSJ_AWAIBA_SetAnalogueGain(int nChannel, KSJ_AWAIBA_ANALOGUE_GAIN Gain);
	KSJ_API  int __stdcall KSJ_AWAIBA_GetAnalogueGain(int nChannel, KSJ_AWAIBA_ANALOGUE_GAIN *pGain);
	*/



#ifdef __cplusplus
}
#endif

#endif