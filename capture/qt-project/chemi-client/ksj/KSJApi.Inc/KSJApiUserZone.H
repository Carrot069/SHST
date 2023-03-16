#ifndef __KSJ_API_USER_ZONE_H__
#define __KSJ_API_USER_ZONE_H__

// #pragma message("Include KSJApiUserZone.h") 

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 参考“凯视佳工业相机内部API函数说明书（用户读写区域篇）”
enum KSJ_EEPROM_TYPE { KSJ_24LC64, KSJ_24LC1024, KSJ_24LC128 };

KSJ_API  int __stdcall KSJ_EEPROMTypeGet( int nChannel, KSJ_EEPROM_TYPE *pType );

KSJ_API  int __stdcall KSJ_WrEEPROM( int nChannel, unsigned short uRomAdress, unsigned char btValue   );
KSJ_API  int __stdcall KSJ_RdEEPROM( int nChannel, unsigned short uRomAdress, unsigned char *pbtValue );

KSJ_API  int __stdcall KSJ_WrEEPROMEx( int nChannel, unsigned long ulRomAdress, unsigned char btValue   );
KSJ_API  int __stdcall KSJ_RdEEPROMEx( int nChannel, unsigned long ulRomAdress, unsigned char *pbtValue );

KSJ_API  int __stdcall KSJ_WrEEPROMExEx(int nChannel, unsigned long ulRomAdress, unsigned char *pData, int nBytes);
KSJ_API  int __stdcall KSJ_RdEEPROMExEx(int nChannel, unsigned long ulRomAdress, int nBytes, unsigned char *pData);


#ifdef __cplusplus
}
#endif

#endif
