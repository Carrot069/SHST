#ifndef MINIDUMP_H
#define MINIDUMP_H

#include <QDebug>
#ifdef QT_DEBUG
#include <QStandardPaths>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QProcess>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <DbgHelp.h>
#include <memory>

#include "WzGlobalConst.h"

LONG WINAPI ApplicationCrashHandler(EXCEPTION_POINTERS *pException);
void InstallMiniDump();
#endif
#endif

#endif // MINIDUMP_H
