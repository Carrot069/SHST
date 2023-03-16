#ifndef SHSTSERVERINFO_H
#define SHSTSERVERINFO_H

#include <QObject>
#include <QDebug>

#include "WzUdpBroadcastReceiver.h"

class ShstServerInfo : public QObject
{
    Q_OBJECT
public:
    explicit ShstServerInfo(QObject *parent = nullptr);
    ~ShstServerInfo() override;

    Q_PROPERTY(QString serverHost READ serverHost() CONSTANT)
    Q_PROPERTY(int serverPort READ serverPort() CONSTANT)
    Q_PROPERTY(int fileServerPort READ fileServerPort() CONSTANT)

private:
    QString serverHost() const;
    int serverPort() const;
    int fileServerPort() const;
signals:

};

#endif // SHSTSERVERINFO_H
