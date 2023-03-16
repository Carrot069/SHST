#ifndef WZLOGGER_H
#define WZLOGGER_H

#include <QObject>
#include <QDebug>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif
#include "WzGlobalConst.h"

class WzLogger : public QObject
{
    Q_OBJECT
public:
    explicit WzLogger(QObject *parent = nullptr);

    Q_INVOKABLE bool startDbgView();
    Q_INVOKABLE void stopDbgView();

private:
#ifdef Q_OS_WIN
    const QString dbgViewFileName = "dbgview.exe";
    bool listProcessModules(DWORD dwPID, const QString &modName, QString &fullName);
    void printError(const QString& msg);
    bool dbgViewExists();
#endif
signals:

};

#endif // WZLOGGER_H
