#ifndef __KSJ_API_BASE_H__
#define __KSJ_API_BASE_H__

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
	// Device Type
	enum KSJ_DEVICETYPE
	{
		KSJ_UC130C_MRNN = 0,   // 0 Guass2
		KSJ_UC130M_MRNN,       // 1 Guass2
		KSJ_RESERVED0,         // 2 Not Support
		KSJ_UC320C_MRNN,       // 3 Guass2
		KSJ_UC130C_MRYN,       // 4 Guass2
		KSJ_UC130M_MRYN,       // 5 Guass2
		KSJ_RESERVED1,         // 6 Not Support
		KSJ_UC320C_MRYN,       // 7 Guass2
		KSJ_UC500C_MRNN,       // 8 Guass2
		KSJ_UC500M_MRNN,       // 9 Guass2
		KSJ_UC500C_MRYN,       // 10 Guass2
		KSJ_UC500M_MRYN,       // 11 Guass2
		KSJ_UC320C_OCR,        // 12 Not Support
		KSJ_UC900C_MRNN,       // 13 Not Support
		KSJ_UC1000C_MRNN,      // 14 Not Support
		KSJ_UC900C_MRYN,       // 15 Not Support
		KSJ_UC1000C_MRYN,      // 16 Not Support
		KSJ_MUC120C_MRYYO,     // 17 Jelly2 V6.0.121.16
		KSJ_MUC120M_MRYYO,     // 18 Jelly2 V6.0.121.16
		KSJ_UD140C_SGNN,       // 19 Not Support
		KSJ_UD140M_SGNN,       // 20 Not Support
		KSJ_UC36C_MGNN,        // 21 Elanus2 Not Support
		KSJ_UC36M_MGNN,        // 22 Elanus2 Not Support
		KSJ_UC36C_MGYN,        // 23 Elanus2 Not Support
		KSJ_UC36M_MGYN,        // 24 Elanus2 Not Support
		KSJ_UC900C_MRYY,       // 25 Elanus2
		KSJ_UC1000C_MRYY,      // 26 Elanus2
		KSJ_UC1400C_MRYY,      // 27 Elanus2
		KSJ_UC36C_MGYY,        // 28 Elanus2 Not Support
		KSJ_UC36M_MGYY,        // 29 Elanus2 Not Support
		KSJ_UC320C_MRYY,       // 30 Elanus2 Not Support
		KSJ_UC500C_MRYY,       // 31 Elanus2 Not Support
		KSJ_UC500M_MRYY,       // 32 Elanus2 Not Support
		KSJ_MUC130C_MRYN,      // 33 OEM
		KSJ_MUC130M_MRYN,      // 34 OEM.
		 
		KSJ_MUC320C_MRYN,      // 35 OEM
		KSJ_MUC36C_MGYYO,      // 36 Jelly2
		KSJ_MUC36M_MGYYO,      // 37 Jelly2 
		KSJ_MUC130C_MRYY,      // 38 Not Support
		KSJ_MUC130M_MRYY,      // 39 Not Support
		KSJ_MUC320C_MRYY,      // 40 Not Support
		KSJ_MUC500C_MRYYO,     // 41 Jelly2
		KSJ_MUC500M_MRYYO,     // 42 Jelly2
		KSJ_MUC900C_MRYY,      // 43 Not Support
		KSJ_MUC1000C_MRYY,     // 44 Not Support
		KSJ_MUC1400C_MRYY,     // 45 Not Support
		KSJ_UD205C_SGYY,       // 46 Elanus2
		KSJ_UD205M_SGYY,       // 47 Elanus2
		KSJ_UD274C_SGYY,       // 48 Elanus2
		KSJ_UD274M_SGYY,       // 49 Elanus2
		KSJ_UD285C_SGYY,       // 50 Elanus2
		KSJ_UD285M_SGYY,       // 51 Elanus2
		KSJ_MU3C500C_MRYYO,    // 52 Jelly3 
		KSJ_MU3C500M_MRYYO,    // 53 Jelly3
		KSJ_MU3C1000C_MRYYO,   // 54 Jelly3
		KSJ_MU3C1400C_MRYYO,   // 55 Jelly3
		KSJ_MU3V130C_CGYYO,    // 56 Jelly3
		KSJ_MU3V130M_CGYYO,    // 57 Jelly3
		KSJ_MU3E130C_EGYYO,    // 58 Jelly3
		KSJ_MU3E130M_EGYYO,    // 59 Jelly3
		KSJ_MUC36C_MGYFO,      // 60 Jelly1
		KSJ_MUC36M_MGYFO,      // 61 Jelly1
		KSJ_MU3C120C_MGYYO,    // 62 Jelly3
		KSJ_MU3C120M_MGYYO,    // 63 Jelly3
		KSJ_MU3E200C_EGYYO,    // 64 Jelly3
		KSJ_MU3E200M_EGYYO,    // 65 Jelly3
		KSJ_MUC130C_MRYNO,     // 66 Jelly1
		KSJ_MUC130M_MRYNO,     // 67 Jelly1
		KSJ_MUC320C_MRYNO,     // 68 Jelly1
		KSJ_U3C130C_MRYNO,     // 69 Not Support
		KSJ_U3C130M_MRYNO,     // 70 Not Support
		KSJ_U3C320C_MRYNO,     // 71 Not Support
		KSJ_U3C500C_MRYNO,     // 72 Guass3
		KSJ_U3C500M_MRYNO,     // 73 Guass3
		KSJ_MU3C1401C_MRYYO,   // 74 DELETE Not Support 
		KSJ_MU3C1001C_MRYYO,   // 75 DELETE Not Support
		KSJ_MUC131M_MRYN,      // 76 OEM Device
		KSJ_MU3C501C_MRYYO,    // 77 DELETE Not Support
		KSJ_MU3C501M_MRYYO,    // 78 DELETE Not Support
		KSJ_MUC120C_MGYYO,     // 79 Jelly2
		KSJ_MUC120M_MGYYO,     // 80 Jelly2
		KSJ_MU3E131C_EGYYO,    // 81 DELETE Not Support
		KSJ_MU3E131M_EGYYO,    // 82 DELETE Not Support
		KSJ_MU3E201C_EGYYO,    // 83 DELETE Not Support
		KSJ_MU3E201M_EGYYO,    // 84 DELETE Not Support   
		KSJ_MISSING_DEVICE,	   // 85 Device Lost Program
		KSJ_MU3S230C_SGYYO,    // 86 Jelly3 Sony IMX174
		KSJ_MU3S230M_SGYYO,    // 87 Jelly3 Sony IMX174
		KSJ_MU3S640C_SRYYO,    // 88 Jelly3 Sony IMX178
		KSJ_MU3S640M_SRYYO,    // 89 Jelly3 Sony IMX178
		KSJ_CUD285C_SGYYO,     // 90
		KSJ_CUD285M_SGYYO,     // 91
		KSJ_MU3S231C_SGYYO,    // 92 Jelly3 Sony IMX249
		KSJ_MU3S231M_SGYYO,    // 93 Jelly3 Sony IMX249
		KSJ_MU3S500C_SGYYO,    // 94
		KSJ_MU3S500M_SGYYO,    // 95
		KSJ_MU3S1200C_SRYYO,   // 96
		KSJ_MU3S1200M_SRYYO,   // 97
		KSJ_MU3L2K7C_AGYYO,    // 98
		KSJ_MU3L2K7M_AGYYO,    // 99
		KSJ_MU3L4K3C_AGYYO,    // 100
		KSJ_MU3L4K3M_AGYYO,    // 101
		KSJ_MU3HS2000C_SRYYO,  // 102
		KSJ_MU3HS2000M_SRYYO,  // 103
		KSJ_MU3HS500C_SGYYO,   // 104
		KSJ_MU3HS500M_SGYYO,   // 105
		KSJ_MU3HS230C_SGYYO,   // 106 imx174 
		KSJ_MU3HS230M_SGYYO,   // 107
		KSJ_MU3HI400C_IGYYO,   // 108
		KSJ_MU3HI400M_IGYYO,   // 109
		KSJ_TEST_CAMERA,       // 110
		KSJ_MU3HI401C_IGYYO, 
		KSJ_MU3HI401M_IGYYO,
		KSJ_MU3S40C_SGYYO, 
		KSJ_MU3S40M_SGYYO,     // 114
		KSJ_MU3S210C_SRYYO,    // 115
		KSJ_MU3S210M_SRYYO,    // 116
		KSJ_MU3I130C_IGYYO,    // 117
		KSJ_MU3I130M_IGYYO,    // 118
		KSJ_MU3S321C_SGYYO,    // 119
		KSJ_MU3S321M_SGYYO,     // 120
		DT_CB_MU3S641M_SRYYO,
		DT_CB_MU3S1201M_SRYYO,
		DT_CB_MU3HS2001M_SRYYO,
		DT_CB_MU3S211M_SRYYO,
		DT_CB_MU3C36C_MGYY,
		DT_CB_MU3C36M_MGYY,
		DT_CB_MU3HS1200C_SRYYO,
		DT_CB_MU3HS1200M_SRYYO

	};
	// Enable or Disable Log output
	KSJ_API  int __stdcall KSJ_LogSet(bool bEnable, const TCHAR *pszFolder);
	// Get Log Status
	KSJ_API  int __stdcall KSJ_LogGet(bool *bEnable, TCHAR *pszFolder);

	// Get KSJApi.dll Version, call any time.
	KSJ_API  int __stdcall KSJ_GetVersion(OUT int *pnMaj1, OUT int *pnMaj2, OUT int *pnMin1, OUT int *pnMin2);
	// Get Return String Information. call any time.
	KSJ_API  int __stdcall KSJ_GetErrorInfo(IN int nErrorNo, OUT TCHAR pszErrorInfo[256]);
	// Initial KSJApi.dll. This Function don't Open Device, only get connected Device Number.
	KSJ_API  int __stdcall KSJ_Init(void);
	//KSJ_API  int __stdcall KSJ_InitEx(int nMaj, int nMin);

	// Uninitial KSJApi.dll. This should call correspond to KSJ_Init.
	KSJ_API  int __stdcall KSJ_UnInit(void);
	// Get Connected Device Number, Must call after KSJ_Init.
	KSJ_API  int __stdcall KSJ_DeviceGetCount(void);
	// Get Device Information.
	KSJ_API  int __stdcall KSJ_DeviceGetInformation(IN int nIndex, OUT KSJ_DEVICETYPE *pDeviceType, OUT int *pnSerials, OUT unsigned short *pwFirmwareVersion);
	// Get Device Information.
	KSJ_API  int __stdcall KSJ_DeviceGetInformationEx(IN int nIndex, OUT KSJ_DEVICETYPE *pDeviceType, OUT int *pnSerials, OUT unsigned short *pwFirmwareVersion, OUT unsigned short *pwFpgaVersion);
	// Device Parameter Settings
	enum KSJ_PARAM
	{
		KSJ_EXPOSURE = 0,        // Exposure Time (ms)
		KSJ_RED,                 // Red Gain, for line scan sensor map to seg0, for ccd map to KSJ_VGAGAIN
		KSJ_GREEN,               // Green Gain, for line scan sensor map to seg1, for ccd map to KSJ_LAMPLEVEL
		KSJ_BLUE,                // Blue Gain, for CCD map to KSJ_CDSGAIN
		KSJ_GAMMA,               // Gamma
		KSJ_PREVIEW_COLUMNSTART, // Preview Col Start
		KSJ_PREVIEW_ROWSTART,    // Preview Row Start
		KSJ_CAPTURE_COLUMNSTART, // Capture Col Start
		KSJ_CAPTURE_ROWSTART,    // Capture Row Start
		KSJ_HORIZONTALBLANK,     // Horizontal Blank
		KSJ_VERTICALBLANK,       // Vertical Blank
		KSJ_FLIP,                // Flip
		KSJ_BIN,                 // Binning
		KSJ_MIRROR,              // Mirror
		KSJ_CONTRAST,            // Contrast
		KSJ_BRIGHTNESS,          // Brightness
		KSJ_VGAGAIN,             // VGA Gain(CCD)
		KSJ_CLAMPLEVEL,          // Clamp Level(CCD)
		KSJ_CDSGAIN,             // CDS Gain(CCD)
		KSJ_RED_SHIFT,           // Not Use
		KSJ_GREEN_SHIFT,         // Not Use
		KSJ_BLUE_SHIFT,          // Not Use
		KSJ_COMPANDING,          // Companding
		KSJ_EXPOSURE_LINES,      // Exposure Lines
		KSJ_SATURATION,          // Saturation
		KSJ_TRIGGERDELAY,        // Trigger Delay Step = 100uS
		KSJ_STROBEDELAY,         // Not Use
		KSJ_TRIGGER_MODE,        // Trigger Mode
		KSJ_TRIGGER_METHOD,      // Trigger Method
		KSJ_BLACKLEVEL,          // Black Level
		KSJ_BLACKLEVEL_THRESHOLD_AUTO, // Black Level Threshold Auto
		KSJ_BLACKLEVEL_THRESHOLD_LO,   // Black Level Low Threshold
		KSJ_BLACKLEVEL_THRESHOLD_HI    // Black Level High Threshold
	};

	// Get Settings Range
	KSJ_API  int __stdcall KSJ_GetParamRange(IN int nIndex, IN KSJ_PARAM Param, OUT int *pnMinValue, OUT int *pnMaxValue);
	// Set Parameter
	KSJ_API  int __stdcall KSJ_SetParam(IN int nIndex, IN KSJ_PARAM Param, IN  int nValue);
	// Get Current Parameter Value
	KSJ_API  int __stdcall KSJ_GetParam(IN int nIndex, IN KSJ_PARAM Param, OUT int *pnValue);

	// Set Exposure Time (ms) for float format.( more precision )
	KSJ_API  int __stdcall KSJ_ExposureTimeSet(int nIndex, float fExposureTimeMs);
	// Get Exposure Time (ms) for float format.
	KSJ_API  int __stdcall KSJ_ExposureTimeGet(int nIndex, float *pfExpTimeMs);

	KSJ_API  int __stdcall KSJ_GlobalResetShutterSet(int nIndex, bool bGlobalReset);
	KSJ_API  int __stdcall KSJ_GlobalResetShutterGet(int nIndex, bool* pbGlobalReset);
	
	// Set Pre-Exposure Time (ms) for float format.( more precision )
	KSJ_API  int __stdcall KSJ_PreExposureRangeGet(int nIndex, int *pnMinValue, int *pnMaxValue);

	// Set Pre-Exposure Time (ms) for float format.( more precision )
	KSJ_API  int __stdcall KSJ_PreExposureTimeSet(int nIndex, float fPreExpTimeMs);
	// Get Pre-Exposure Time (ms) for float format.
	KSJ_API  int __stdcall KSJ_PreExposureTimeGet(int nIndex, float *pfPreExpTimeMs);

	// Set Pre-Exposure Lines for float format.( more precision )
	KSJ_API  int __stdcall KSJ_PreExposureLinesSet(int nIndex, int nPreExpLines);
	// Get Pre-Exposure Lines for float format.
	KSJ_API  int __stdcall KSJ_PreExposureLinesGet(int nIndex, int* pnPreExpLines);

	// Skip Mode (Address Mode)
	enum KSJ_ADDRESSMODE 
	{ 
		KSJ_SKIPNONE = 0, 
		KSJ_SKIP2, 
		KSJ_SKIP3, 
		KSJ_SKIP4,
		KSJ_SKIP8 
	};
	///////////////////////////////////////////Capture/////////////////////////////////
	// Get The Max Capture FOV( Field Of View £© supported by device
	KSJ_API  int __stdcall KSJ_CaptureGetDefaultFieldOfView(int nIndex, int *pnColStart, int *pnRowStart, int *pnColSize, int *pnRowSize, KSJ_ADDRESSMODE *pColAddressMode, KSJ_ADDRESSMODE *pRowAddressMode);
	// Set Capture FOV/AOI( Field Of View / Area Of Interesting £©
	KSJ_API  int __stdcall KSJ_CaptureSetFieldOfView(int nIndex, int nColStart, int nRowStart, int nColSize, int nRowSize, KSJ_ADDRESSMODE AmCol, KSJ_ADDRESSMODE AmRow);
	// Get Current Capture FOV / AOI( Field Of View / Area Of Interesting £©
	KSJ_API  int __stdcall KSJ_CaptureGetFieldOfView(int nIndex, int *pnColStart, int *pnRowStart, int *pnColSize, int *pnRowSize, KSJ_ADDRESSMODE *pColAddressMode, KSJ_ADDRESSMODE *pRowAddressMode);
	// Get Capture Image's Width and Height in Pixel Unit.
	KSJ_API  int __stdcall KSJ_CaptureGetSize(int nIndex, int *pnWidth, int *pnHeight);
	// Get Capture Image's Width, Height in Pixel Unit and BitCount( 8, 24, 32 )
	KSJ_API  int __stdcall KSJ_CaptureGetSizeEx(int nIndex, int *pnWidth, int *pnHeight, int *pnBitCount);
	// Capture Raw Data. ( Note: When Set 16bits, Buffer Should double )
	KSJ_API  int __stdcall KSJ_CaptureRawData(int nIndex, unsigned char *pRawData);
	// Capture 8, 24, 32 Format Image Data.( Note: When Set 16bits, Buffer Should double )
	KSJ_API  int __stdcall KSJ_CaptureRgbData(int nIndex, unsigned char *pRgbData);
	// Capture 8, 24, 32 Format Image Data.( Note: When Set 16bits, Buffer Should double )
	KSJ_API  int __stdcall KSJ_CaptureRgbDataEx(int nIndex, unsigned char *pRgbData, int *pnWidth, int *pnHeight, int *pnBitCount);
	// Save to Bmp Format
	KSJ_API  int __stdcall KSJ_HelperSaveToBmp(unsigned char *pData, int nWidth, int nHeight, int nBitCount, const TCHAR *pszFileName);
	// Save to Jpg Format
	KSJ_API  int __stdcall KSJ_HelperSaveToJpg(unsigned char *pData, int nWidth, int nHeight, int nBitCount, int nQulity, const TCHAR *pszFileName);

	///////////////////////////////////////////Preview/////////////////////////////////
	// Get The Max Preview FOV( Field Of View £© supported by device
	KSJ_API  int __stdcall KSJ_PreviewGetDefaultFieldOfView(int nIndex, int *pnColStart, int *pnRowStart, int *pnColSize, int *pnRowSize, KSJ_ADDRESSMODE *pColAddressMode, KSJ_ADDRESSMODE *pRowAddressMode);
	// Set Preview FOV/AOI( Field Of View / Area Of Interesting £©
	KSJ_API  int __stdcall KSJ_PreviewSetFieldOfView(int nIndex, int nColStart, int nRowStart, int nColSize, int nRowSize, KSJ_ADDRESSMODE AmCol, KSJ_ADDRESSMODE AmRow);
	// Get Current Preview FOV / AOI( Field Of View / Area Of Interesting £©
	KSJ_API  int __stdcall KSJ_PreviewGetFieldOfView(int nIndex, int *pnColStart, int *pnRowStart, int *pnColSize, int *pnRowSize, KSJ_ADDRESSMODE *pColAddressMode, KSJ_ADDRESSMODE *pRowAddressMode);
	// Get Preivew Image's Width and Height in Pixel Unit.
	KSJ_API  int __stdcall KSJ_PreviewGetSize(int nIndex, int *pnWidth, int *pnHeight);
	// Get Preview Image's Width, Height in Pixel Unit and BitCount( 8, 24, 32 )
	KSJ_API  int __stdcall KSJ_PreviewGetSizeEx(int nIndex, int *pnWidth, int *pnHeight, int *pnBitCount);
	// Set Preview Position
	KSJ_API  int __stdcall KSJ_PreviewSetPos(int nIndex, HWND hWnd, int x, int y, int nWidth, int nHeight);
	// Get Preview Position
	KSJ_API  int __stdcall KSJ_PreviewGetPos(int nIndex, int *px, int *py, int *pnWidth, int *pnHeight);
	// Preview Mode
	enum KSJ_PREVIEWMODE
	{
		PM_RAWDATA, 
		PM_RGBDATA
	};
	// Set Preview Mode
	KSJ_API  int __stdcall KSJ_PreviewSetMode(int nIndex, KSJ_PREVIEWMODE PreviewMode);
	// Get Preview Mode
	KSJ_API  int __stdcall KSJ_PreviewGetMode(int nIndex, KSJ_PREVIEWMODE *pPreviewMode);
	// Get Default Mode
	KSJ_API  int __stdcall KSJ_PreviewGetDefaultMode(int nIndex, KSJ_PREVIEWMODE *pPreviewMode);
    // Start or Stop Preview
	KSJ_API  int __stdcall KSJ_PreviewStart(int nIndex, bool bStart);
	// Pause or Resume Preview
	KSJ_API  int __stdcall KSJ_PreviewPause(int nIndex, bool bPause);
	enum    KSJ_PREVIEWSTATUS 
	{
		PS_STOP,
		PS_START,
		PS_PAUSE
	};
	// Get Preview Status
	KSJ_API  int __stdcall KSJ_PreviewGetStatus(int nIndex, KSJ_PREVIEWSTATUS *pPreviewStatus);
	// Get Preview Frame Rate
	KSJ_API  int __stdcall KSJ_PreviewGetFrameRate(int nIndex, float *fFrameRate);
	// Get Paused Image Data when PS_PAUSE Status
	KSJ_API  int __stdcall KSJ_PreviewGetPausedData(int nIndex, unsigned char *pData);
	// Get Paused Image Data when PS_PAUSE Status
	KSJ_API  int __stdcall KSJ_PreviewGetPausedDataEx(int nIndex, unsigned char *pData, int *pnWidth, int *pnHeight, int *pnBitCount);

	// Captrue and Show may Parallel
	KSJ_API  int __stdcall KSJ_PreviewStartEx(int nIndex, bool bStart, bool bParallel );

	KSJ_API  int __stdcall KSJ_PreviewGetStatusEx(int nIndex, KSJ_PREVIEWSTATUS *pPreviewStatus, bool *pbParallel);
	// Get Capture and Show Frame rate when Preview Parallel.
	KSJ_API  int __stdcall KSJ_PreviewGetFrameRateEx(int nIndex, float *fFrameRateCapture, float *fFrameRateShow);
	KSJ_API  int __stdcall KSJ_GetPreviewFrameCount(int nIndex, int *pnCaptureFrameCount, int *pnShowFrameCount);

	KSJ_API  int __stdcall KSJ_SetBufferCount(int nIndex, int nCount);
	KSJ_API  int __stdcall KSJ_GetBufferCount(int nIndex, int* pnCount);

	enum KSJ_USBSPEED
	{
		USB_NOT_CONNECTED = 0x00,        /**< USB device not connected. */
		USB_FULL_SPEED,                  /**< USB full speed. */
		USB_HIGH_SPEED,                  /**< High speed. */
		USB_SUPER_SPEED                  /**< Super speed. */
	};

	KSJ_API  int __stdcall KSJ_GetUsbState(int nIndex, KSJ_USBSPEED* pUSBSpeed);

	KSJ_API  int __stdcall KSJ_RebootDevice(int nIndex);


#ifdef __cplusplus
}
#endif

#endif
