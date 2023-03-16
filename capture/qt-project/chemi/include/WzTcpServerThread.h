#ifndef WZTCPSERVERTHREAD_H
#define WZTCPSERVERTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QMutex>
#include <QWaitCondition>
#include <QtNetwork>

class WzTcpServerThread : public QThread
{
    Q_OBJECT

public:
    WzTcpServerThread(int socketDescriptor, QByteArray *data, QMutex *dataMutex,
                  QWaitCondition *dataCondition, QObject *parent);

    ~WzTcpServerThread() override;

    void run() override;

    void abort();

signals:
    void error(QTcpSocket::SocketError socketError);

private:
    int m_socketDescriptor;
    bool m_abort = false;
    QByteArray* m_data;
    QMutex* m_dataMutex;
    QWaitCondition* m_dataCondition;
};

#endif
