#ifndef WZTCPSERVER_H
#define WZTCPSERVER_H

#include <QTcpServer>
#include <QWaitCondition>
#include <QMutex>
#include <QList>

#include "WzTcpServerThread.h"

class WzTcpServer : public QTcpServer
{
    Q_OBJECT

public:
    WzTcpServer(QObject *parent = 0);
    ~WzTcpServer() override;
    void writeData(QByteArray data);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void threadFinished();

private:
    QByteArray m_data;
    QMutex m_dataMutex;
    QWaitCondition m_dataCondition;
    QList<WzTcpServerThread*> m_threads;
};

#endif
