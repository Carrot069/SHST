#ifndef WZLOCALSERVER_H
#define WZLOCALSERVER_H

#include <QCoreApplication>
#include <QLocalServer>
#include <QObject>
#include <QLocalSocket>

#include "WzLocalServerService.h"

class WzLocalServer : public QLocalServer
{
    Q_OBJECT
public:
    WzLocalServer(QObject *parent = nullptr);
protected:
    void incomingConnection(quintptr socketDescriptor) override;
signals:
    void action(const QJsonDocument &jsonDocument);
};

extern WzLocalServer *g_localServer;

#endif // WZLOCALSERVER_H
