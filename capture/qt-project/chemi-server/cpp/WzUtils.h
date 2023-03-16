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

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

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
    Q_INVOKABLE static bool isGelCapture();
    Q_INVOKABLE static bool isChemiCapture();
    Q_INVOKABLE static bool isTcpClient();
    static bool aesDecryptFileToBuf(aes256_key key, QString fileName, char** buffer, int* bufrerSize);
    Q_INVOKABLE static void sleep(int ms);
    Q_INVOKABLE static QString desktopPath();
    Q_INVOKABLE static void openConfigPath();

    Q_INVOKABLE static void takeCrash();
};

#endif // WZUTILS_H
