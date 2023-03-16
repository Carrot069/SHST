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
#include <QRect>
#include <QStandardPaths>

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
    static void calc16bitTo8bit(uint16_t* buffer, uint32_t count, uint8_t* mapTable);
    Q_INVOKABLE static void processEvents();
    Q_INVOKABLE static void copyToClipboard(const QString& text);
    Q_INVOKABLE static bool validatePath(const QString& path);
    Q_INVOKABLE static QString toLocalPath(const QString& path);
    Q_INVOKABLE static QString toLocalFile(const QString& fileName);
    Q_INVOKABLE static QString appName();
    Q_INVOKABLE static QString appVersion();
    Q_INVOKABLE static bool isGelCapture();
    Q_INVOKABLE static bool isNoLogo();

    static bool aesDecryptFileToBuf(aes256_key key, QString fileName, char** buffer, int* bufrerSize);
    Q_INVOKABLE static void sleep(int ms);
    Q_INVOKABLE static QString desktopPath();

    // 比较两个矩形，将内部矩形放大或缩小, 适应外部矩形.
    // 本函数会将内部矩形做居中计算, 不论内部矩形是否大于外部矩形都如此
    // scale: 内部矩形的缩放比例, 100表示未缩放, 小于100表示缩小了, 大于100表示放大了
    // allowZoomOut: 内部矩形的高或宽大于外部矩形时允许内部矩形缩小
    // allowZoomIn: 内部矩形的高和宽都小于外部矩形时允许内部矩形放大
    static QRect fitRect(const QRect &outerRect, const QRect &innerRect,
                         double &scale,
                         const bool allowZoomOut = true, const bool allowZoomIn = true);

    static QRect absRect(const QPoint &p1, const QPoint &p2);
    static QRect absRect(const QRect &r);
    // 使p1的x/y坐标始终最小, p2的始终最大
    static void absPoints(QPoint& p1, QPoint& p2);
    // 计算两个Point的角度
    static double getAngle(const QPoint& p1, const QPoint& p2);

    static void RC(uint16_t* buf, int width, int height, int x, int y, uint16_t &rgb, const uint16_t &defaultColor);
    static void RC(uint8_t* buf, int width, int height, int x, int y, uint8_t &rgb, const uint8_t &defaultColor);
    static uint16_t Bilinear(uint16_t* buf, int width, int height, double x, double y, const uint16_t &defaultColor);
    static uint8_t Bilinear(uint8_t* buf, int width, int height, double x, double y, const uint8_t &defaultColor);

    static QRect zoomRect(const QRect &rect, const qreal zoom);
    static QPoint zoomPoint(const QPoint &point, const qreal zoom);
    static QPoint noZoomPoint(const QPoint &point, const qreal zoom);
};

#endif // WZUTILS_H
