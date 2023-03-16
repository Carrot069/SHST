#ifndef WZJAVASERVICE_H
#define WZJAVASERVICE_H

#include <QObject>
#include <QDebug>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidIntent>
#endif

class WzJavaService: public QObject
{
    Q_OBJECT
public:
    WzJavaService(QObject *parent = nullptr);
    Q_INVOKABLE static void connectWiFi(const QString &wifiName, const QString &wifiPassword);
private:
};

#endif // WZJAVASERVICE_H
