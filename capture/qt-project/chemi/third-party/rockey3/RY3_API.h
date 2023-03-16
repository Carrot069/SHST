#ifndef  FT_RY3_HEADER_H
#define  FT_RY3_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif
typedef void*  RY_HANDLE;

//错误码
#define   RY3_SUCCESS                               0x00000000        // 操作成功
#define   RY3_NOT_FOUND                             0xF0000001        // 未找到指定的设备
#define   RY3_INVALID_PARAMETER                     0xF0000002        // 参数错误
#define   RY3_COMM_ERROR                            0xF0000003        // 通讯错误
#define   RY3_INSUFFICIENT_BUFFER                   0xF0000004        // 缓冲区空间不足
#define   RY3_NO_LIST                               0xF0000005        // 没有找到设备列表
#define   RY3_DEVPIN_NOT_CHECK                      0xF0000006        // 开发商口令没有验证
#define   RY3_USERPIN_NOT_CHECK                     0xF0000007        // 用户口令没有验证
#define   RY3_RSA_FILE_FORMAT_ERROR                 0xF0000008        // RSA文件格式错误
#define   RY3_DIR_NOT_FOUND                         0xF0000009        // 目录没有找到
#define   RY3_ACCESS_DENIED                         0xF000000A        // 访问被拒绝
#define   RY3_ALREADY_INITIALIZED                   0xF000000B        // 产品已经初始化
#define   RY3_INCORRECT_PIN                         0xF0000C00        // 密码不正确
#define   RY3_DF_SIZE                               0xF000000D        // 指定的目录空间大小不够
#define   RY3_FILE_EXIST                            0xF000000E        // 文件已存在
#define   RY3_UNSUPPORTED                           0xF000000F        // 功能不支持或尚未建立文件系统
#define   RY3_FILE_NOT_FOUND                        0xF0000010        // 未找到指定的文件
#define   RY3_ALREADY_OPENED                        0xF0000011        // 卡已经被打开
#define   RY3_DIRECTORY_EXIST                       0xF0000012        // 目录已存在
#define   RY3_CODE_RANGE                            0xF0000013        // 虚拟机内存地址溢出
#define   RY3_INVALID_POINTER                       0xF0000014        // 虚拟机错误的指针
#define   RY3_GENERAL_FILESYSTEM                    0xF0000015        // 常规文件系统错误
#define   RY3_OFFSET_BEYOND                         0xF0000016        // 文件偏移量超出文件大小
#define   RY3_FILE_TYPE_MISMATCH                    0xF0000017        // 文件类型不匹配
#define   RY3_PIN_BLOCKED                           0xF0000018        // PIN码锁死
#define   RY3_INVALID_HANDLE                        0xF0000019        // 无效的句柄
#define   RY3_ERROR_UNKNOWN                         0xFFFFFFFF        // 未知的错误


#define   RY3_C51_SUCCESS                           0x00000000            // 成功
#define   RY3_C51_UNKNOWN                           0x00000001            // 未知错误
#define   RY3_C51_INVALID_PARAMETER                 0x00000002            // 无效的参数
#define   RY3_C51_INVALID_ADDRESS            0x00000003            // 无效的地址,虚拟机地址越界
#define   RY3_C51_INVALID_SIZE                0x00000004            // 无效的长度
#define   RY3_C51_FILE_NOT_FOUND            0x00000005            // 文件没找到
#define   RY3_C51_ACCESS_DENIED                     0x00000006            // 访问文件失败
#define   RY3_C51_FILE_SELECT                0x00000007            // 文件打开个数已达上限
#define   RY3_C51_INVALID_HANDLE            0x00000008            // 无效的文件句柄
#define   RY3_C51_FILE_OUT_OF_RANGE                 0x00000009            // 文件读写越界
#define   RY3_C51_FILE_TYPE_MISMATCH                0x0000000A            // 文件存在但类型不匹配
#define   RY3_C51_FILE_SIZE_MISMATCH                0x0000000B            // 文件存在但长度不匹配
#define   RY3_C51_NO_SPACE                          0x0000000C            // 文件夹空间不足
#define   RY3_C51_FILE_EXIST                0x0000000D            // 文件或子目录已存在
#define   RY3_C51_INVALID_KEY_FORMAT                0x0000000E            // 无效的RSA密钥文件格式
#define   RY3_C51_KEY_LEN_MISMATCH            0x0000000F            // 用户传入的密钥长度与实际长度不匹配
#define   RY3_C51_RSA_INVALID_KEY_FILE              0x00000010            // 文件类型不符合要求
#define   RY3_C51_RSA_ENC_DEC_FAILED                0x00000011            // RSA加密解密失败
#define   RY3_C51_RSA_SIGN_VERI_FAILED              0x00000012            // RSA签名验证失败
#define   RY3_C51_SHA1                    0x00000013            // SHA1计算错误
#define   RY3_C51_MD5                    0x00000014            // MD5计算错误
#define   RY3_C51_INVALID_ADDERSS            0x00000015            // 无效的内存指针
#define   RY3_C51_EEPROM                0x00000016            // 写EEPROM错误


typedef struct
{
    int     m_Item;          //基于1的设备条目
    char    m_VendorID[12];  //8字节字符串形式的VendorID
    char    m_HardID[20];    //16字节字符串形式的HardID
}RY3_INFO;


//===============================================================================

//查找RY3 (输入VendorID字串, 输出找到的RY3数目)
unsigned long __stdcall RY3_Find(char* pVendorID, int* pCount);

//打开指定条目的RY3 (输入条目,基于1,输出句柄)
unsigned long __stdcall RY3_Open(RY_HANDLE* pHandle, int Item);

//关闭RY3 (IsReset表示是否复位加密锁,清除安全状态)
unsigned long __stdcall RY3_Close(RY_HANDLE handle, char IsReset);

//设置VendorID (种子码长度应小于250字节)
unsigned long __stdcall RY3_SetVendorID(RY_HANDLE handle, char* pSeed, int len, char* pOutVendorID);

//获取硬件序列号(输出16字节的字符串)
unsigned long __stdcall RY3_GetHardID(RY_HANDLE handle, char* pbuf);

//获取随机数(len指定所需的随机数长度)
unsigned long __stdcall RY3_GenRandom(RY_HANDLE handle, int len_need, unsigned char* pOutbuf);

//获取空闲空间大小
unsigned long __stdcall RY3_GetFreeSize(RY_HANDLE handle, int* pSize);

#define RY3_LED_ON                   1 //灯亮
#define RY3_LED_OFF                  2 //灯灭
#define RY3_LED_WINK                 3 //灯闪

//LED控制
unsigned long __stdcall RY3_LEDControl(RY_HANDLE handle, int flag);

//校验开发商密码
//校验成功时返回的RemainCount  0
//校验失败时返回的RemainCount  0:已锁死  1-254:剩余次数  255:无限制
unsigned long __stdcall RY3_VerifyDevPin(RY_HANDLE handle, char* pInPin, int* pRemainCount);

//更改开发商密码(TryCount取值范围为1-255, 其中255表示无限制)
unsigned long __stdcall RY3_ChangeDevPin(RY_HANDLE handle, char* pOldPin, char* pNewPin, int TryCount);

//读存储区(0-8191字节)
unsigned long __stdcall RY3_Read(RY_HANDLE handle, int offset, unsigned char* pOutbuf, int len);

//写存储区(0-7167字节)
unsigned long __stdcall RY3_Write(RY_HANDLE handle, int offset, unsigned char* pInbuf, int len);

//开发商写存储区(0-8191字节)
unsigned long __stdcall RY3_VendorWrite(RY_HANDLE handle, int offset, unsigned char* pInbuf, int len);

//读锁共享内存区(0-31字节)
unsigned long __stdcall RY3_ReadShare(RY_HANDLE handle, int offset, unsigned char* pbuf, int len);

//写锁共享内存区(0-31字节)
unsigned long __stdcall RY3_WriteShare(RY_HANDLE handle, int offset, unsigned char* pbuf, int len);

#define FILE_TYPE_EXE                 0         //可执行文件
#define FILE_TYPE_DATA                  1         //数据文件
#define FILE_TYPE_RSA_PUBLIC         2         //RSA 公钥文件
#define FILE_TYPE_RSA_PRIVATE         3         //RSA 私钥文件

//创建文件
unsigned long __stdcall RY3_CreateFile(RY_HANDLE handle, unsigned short FileID, int Size, int Type);

//写文件
unsigned long __stdcall RY3_WriteFile(RY_HANDLE handle, unsigned short FileID, int offset, unsigned char* pbuf, int Size);

//执行文件
unsigned long __stdcall RY3_ExecuteFile(RY_HANDLE handle, unsigned short FileID, unsigned char* pInBuf, int InSize, unsigned char* pOutBuf, int* pOutSize);

//删除所有文件
unsigned long __stdcall RY3_EraseAllFile(RY_HANDLE handle);

//产生RSA公私钥(kid取值范围:0-8, 其中0表示升级公私钥对, 1-8为普通公私钥对)
//pPubBakup中输出140字节的公钥供备份
//pPriBakup中输出340字节的私钥供备份
//pPubBakup或pPriBakup为NULL时表示不备份
unsigned long __stdcall RY3_GenRsaKey(RY_HANDLE handle, int kid, unsigned char* pPubBakup, unsigned char* pPriBakup);

//产生RSA密钥对512位
unsigned long __stdcall RY3_GenRsaKey512(RY_HANDLE handle, unsigned short pubID,unsigned short priID, unsigned char* pPubBakup, unsigned char* pPriBakup);

//产生RSA密钥对2048位
unsigned long __stdcall RY3_GenRsaKey2048(RY_HANDLE handle, unsigned short pubID,unsigned short priID, unsigned char* pPubBakup, unsigned char* pPriBakup);

//设置RSA公私钥(kid取值范围:0-8, 其中0表示升级公私钥对, 1-8为普通公私钥对)
//pPubKey中输入140字节的公钥供写入
//pPriKey中输入340字节的私钥供写入
//pPubKey或pPriKey为NULL时表示不写入
unsigned long __stdcall RY3_SetRsaKey(RY_HANDLE handle, int kid, unsigned char* pPubKey, unsigned char* pPriKey);

//公钥加密算法(kid取值范围:1-8)
//输入数据长度必须是128的整数倍
unsigned long __stdcall RY3_PublicEncrypt(RY_HANDLE handle, int kid, unsigned char* pBuf, int len);

//私钥解密算法(kid取值范围:1-8)
//输入数据长度必须是128的整数倍
unsigned long __stdcall RY3_PrivateDecrypt(RY_HANDLE handle, int kid, unsigned char* pBuf, int len);

//MD5签名算法
unsigned long __stdcall RY3_MD5(RY_HANDLE handle, unsigned char* pBuf, int len, unsigned char* pMD5);

//SHA1签名算法
unsigned long __stdcall RY3_SHA1(RY_HANDLE handle, unsigned char* pBuf, int len, unsigned char* pSHA1);

//设置3DES加解密的密钥,密钥长度固定为16字节, (kid取值范围:1-8)
unsigned long __stdcall RY3_Set3DESKey(RY_HANDLE handle, int kid, unsigned char* pKey);

#define FLAG_ENCODE  0 //加密
#define FLAG_DECODE  1 //解密

//3DES加解密算法(kid取值范围:1-8)
//输入数据长度必须是8的整数倍
unsigned long __stdcall RY3_3DES(RY_HANDLE handle, int kid, int flag, unsigned char* pBuf, int len);


//远程升级加密锁内数据(pbuf中是升级公钥加密的UpdateData数据结构, len必须是128字节的整数倍)
unsigned long __stdcall RY3_Update(RY_HANDLE handle, unsigned char* pbuf, int len);

#define Data_TYPE_PubKey   0   //公钥数据
#define Data_TYPE_PriKey   1   //私钥数据
#define Data_TYPE_3DesKey  2   //3DES密钥数据
#define Data_TYPE_Memory   3   //8192字节数据区

//产生升级数据包
//pLicSN:     指向允许升级的锁的硬件序列号,如果pLicSN=NULL表示不限制硬件序列号
//type:       要升级的数据类型
//kid_offset: 要升级的数据条目或偏移地址(为条目时,取值范围为1-8, 当type=Data_TYPE_Memory时,表示8192数据区的偏移地址)
//pbuf:       要升级的数据地址
//len:        要升级的数据长度(注:len必须为140字节(公钥数据),340字节(私钥数据),16字节(ES密钥数据))
//pUPubKey:   指向升级公钥数据
//pOutData:   输出产生的升级数据包
//pOutLen:    输出升级数据包的长度
unsigned long __stdcall RY3_GenUpdatePacket(RY_HANDLE handle, char* pLicSN, int type, int kid_offset, unsigned char* pbuf, int len, unsigned char* pUPubKey, /*unsigned long value,unsigned char policy,*/ unsigned char* pOutData, int* pOutLen);


#ifdef __cplusplus 
}
#endif

#endif    //FT_RY3_HEADER_H
