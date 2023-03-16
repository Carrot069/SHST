#ifndef LAUNCHERHELPER_H
#define LAUNCHERHELPER_H

#include <QObject>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif

class LauncherHelper : public QObject
{
    Q_OBJECT
public:
    explicit LauncherHelper(QObject *parent = nullptr);

    Q_INVOKABLE void started();
    Q_INVOKABLE void exiting();

signals:

};

#endif // LAUNCHERHELPER_H
