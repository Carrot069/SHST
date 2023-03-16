/****************************************************************************
**
** Author     : wangzhe
** Date       : 2021-12-30 16:17:15
** Version    : 1.0
** Description: TCP文件传输
**
****************************************************************************/

#ifndef SHSTFILESERVER_H
#define SHSTFILESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QWaitCondition>
#include <QMutex>
#include <QList>

#include "ShstTcpFileService.h"

class ShstFileServer : public QTcpServer
{
    Q_OBJECT
public:
    ShstFileServer(QObject *parent = nullptr);
    ~ShstFileServer() override;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void serviceFinished();

private:
    QList<ShstTcpFileService*> m_services;

signals:

};

#endif // SHSTFILESERVER_H
