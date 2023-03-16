#ifndef __KSJ_API_MULTI_PROCESS_H__
#define __KSJ_API_MULTI_PROCESS_H__

// #pragma message("Include KSJApiMultiProcess.h") 

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

	// 注意：当无特殊说明时，所有第一个参数为nIndex的函数，在设备对象没有创建的情况下，都会自动创建设备对象并打开设备。

	// 关闭指定索引的设备，如果不关闭指定相机，那么在多进程下是不能操作这台相机的。
	// 虽然KSJ_DeviceClose函数的第一个参数为nIndex，但内部不会隐式创建设备对象并开启设备。
	KSJ_API  int __stdcall KSJ_DeviceClose(int nIndex);

	enum KSJ_DEVICESTATUS { KSJ_DS_CLOSE, KSJ_DS_OPEN };
	// KSJ_DeviceGetStatus这个函数不会自动创建设备对象，只是获取当前设备在当前进程的开启状态。
	KSJ_API  int __stdcall KSJ_DeviceGetStatus(int nIndex, KSJ_DEVICESTATUS *pDs);



#ifdef __cplusplus
}
#endif

#endif