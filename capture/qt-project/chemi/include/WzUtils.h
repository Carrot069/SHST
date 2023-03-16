#ifndef WZUTILS_H
#define WZUTILS_H

#include <iostream>
#include <fstream>
#include <algorithm>
#include <QObject>
#include <QClipboard>
#include <QApplication>
#include <QString>
#include <QTime>
#include <QDebug>
#include <QQmlEngine>
#include <QJSEngine>
#include <QDir>
#include <QElapsedTimer>
#include <QStandardPaths>
#include <QRect>

#ifdef WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include "WzGlobalEnum.h"
#include "WzSingleton.h"
#include "aes/aes.hpp"

const uint8_t IV_LEN = 16;
typedef uint8_t aes_iv[IV_LEN];
const int AES256_KEY_LEN = 32;
typedef uint8_t aes256_key[AES256_KEY_LEN];

class AutoFreeArray {
private:
    uint16_t* buf16bit;
public:
    AutoFreeArray(uint16_t* buf);
    ~AutoFreeArray();
};

class WzUtils: public QObject {
    Q_OBJECT
public:
    WzUtils(QObject* parent = nullptr);
    ~WzUtils() override;

    Q_INVOKABLE static QString getTimeStr(int ms, bool isShowMs, bool isText = false);
    static void calc16bitTo8bit(uint16_t* buffer, uint32_t count, uint8_t* mapTable, const int& maxLimit = 65535);
    Q_INVOKABLE static void processEvents();
    Q_INVOKABLE static void copyToClipboard(const QString& text);
    Q_INVOKABLE static bool validatePath(const QString& path);
    Q_INVOKABLE static QString toLocalPath(const QString& path);
    Q_INVOKABLE static QString toLocalFile(const QString& fileName);
    Q_INVOKABLE static QString appName();
    Q_INVOKABLE static QString appVersion();
    Q_INVOKABLE static bool isOnlyChinese();
    Q_INVOKABLE static bool isGelCapture();
    Q_INVOKABLE static bool isChemiCapture();
    Q_INVOKABLE static bool isTcpClient();
    Q_INVOKABLE static bool isDemo();
    static bool aesDecryptFileToBuf(aes256_key key, QString fileName, char** buffer, int* bufrerSize);
    Q_INVOKABLE static void sleep(int ms);
    Q_INVOKABLE static QString desktopPath();
    Q_INVOKABLE static bool deleteFile(const QString &fileName);
    Q_INVOKABLE static void openPath(const QString &path);
    Q_INVOKABLE static void openPathByFileName(const QString &fileName);
    Q_INVOKABLE static void openDataPath();
    Q_INVOKABLE static void openDbgView();
    Q_INVOKABLE static bool isExposureArea();
    Q_INVOKABLE static bool isMac();
    Q_INVOKABLE static bool isOEM();
    Q_INVOKABLE static bool isOEMJP();
    Q_INVOKABLE static bool isNoLogo();
    Q_INVOKABLE static bool isMini();
    Q_INVOKABLE static bool isMedFilTiff8bit();
    Q_INVOKABLE static QString lightTypeStr(const WzEnum::LightType &lt);
    Q_INVOKABLE static WzEnum::LightType lightTypeFromStr(const QString &s);

    // 比较两个矩形，将内部矩形放大或缩小, 适应外部矩形.
    // 本函数会将内部矩形做居中计算, 不论内部矩形是否大于外部矩形都如此
    // scale: 内部矩形的缩放比例, 100表示未缩放, 小于100表示缩小了, 大于100表示放大了
    // allowZoomOut: 内部矩形的高或宽大于外部矩形时允许内部矩形缩小
    // allowZoomIn: 内部矩形的高和宽都小于外部矩形时允许内部矩形放大
    static QRect fitRect(const QRect &outerRect, const QRect &innerRect,
                         double &scale,
                         const bool allowZoomOut = true, const bool allowZoomIn = true);

    static void new8bitBuf(uint8_t** buf, int width, int height);
    static void new16bitBuf(uint16_t** buf, int width, int height);
    static void new32bitBuf(uint32_t** buf, int width, int height);
    static void delete8bitBuf(uint8_t **buf);
    static void delete16bitBuf(uint16_t **buf);
    static void delete32bitBuf(uint32_t **buf);
    // 把8位数据扩展成24位, 每个8位的数据相同, 典型应用场景: 8位图片需要存为24位tiff
    static uint8_t* bit8BufExpand24(const uint8_t* buf, int w, int h);
};

#endif // WZUTILS_H
