#ifndef	__KSJ_CODE_H__
#define __KSJ_CODE_H__

// #pragma message("Include KSJCode.h") 


/*-------------------------------------------------
	Return Code
  ================================================*/
#define		RET_SUCCESS			    1      // Function Return Successfully.
#define		RET_PARAMERROR			-1     // User's Parameter Passed to API Error.
#define		RET_MALLOCFAIL			-2     // Memory Allocated Fail.
#define		RET_NOTSUPPORT			-3     // Function not supported by such type camera.
#define		RET_DEVICENOTEXIST		-4     // Device doesn't be detected.
#define		RET_DEVICENOTINIT		-5     // Device haven't be initialized
#define		RET_VIOLATION			-6     // This operation is conflict to other operation.
#define		RET_NOPRIVILEGE			-7     // User no privilege
#define		RET_FAIL			    -8     // Function Return Failed.( Normal Error, no Detail )
#define		RET_WRONG			    -9     // Same to RET_FAIL.
#define     RET_RECOVERY_SUCCESS    -10    // Device is recovered successfully.
#define     RET_RECOVERY_FAIL       -11    // Device recovered, but fail.
#define     RET_BADFRAME            -12    // Bad Frame comes from sensor, should be skipped. 
#define     RET_INVALIDFRAME        -13    // Invalid Frame, Transmission Error, this frame should re-get.
#define     RET_ZEROFRAME           -14    // 帧存相机会返回此值，表示采集图像数据0字节，错误的帧
#define     RET_VERSION_ERROR       -15    // 版本错误
#define     RET_TIMEOUT             -16    // 当设置读取超时之后不进行恢复时，采集函数会返回此数值，而不会返回恢复的状态
#define     RET_DEVICECLOSED        -17
#define     RET_BUSNOTINIT          -18    // 总线没有初始化
#define     RET_CAM_NOT_CREATED     -19
#define     RET_INVALID_HANDLE      -20
#define     RET_STREAMNOSTART       -21
#define     RET_3DNOTINIT           -22
#define     RET_3DNOTSUPPORT        -23
#define     RET_STOPSTREAMFIRST     -24
#define     RET_DATAIO_ERROR        -25


#endif