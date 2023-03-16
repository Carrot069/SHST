#ifndef WZFILESERVER_H
#define WZFILESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QWaitCondition>
#include <QMutex>
#include <QList>

#include "WzFileService.h"

class WzFileServer : public QTcpServer
{
    Q_OBJECT
public:
    WzFileServer(QObject *parent = nullptr);
    ~WzFileServer() override;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void serviceFinished();

private:
    QList<WzFileService*> m_services;

signals:

};

#endif // WZFILESERVER_H
