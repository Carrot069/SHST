#include "WzTcpServerThread.h"

WzTcpServerThread::WzTcpServerThread(int socketDescriptor, QByteArray* data, QMutex* dataMutex,
                             QWaitCondition* dataCondition, QObject *parent)
    : QThread(parent),
      m_socketDescriptor(socketDescriptor),
      m_data(data),
      m_dataMutex(dataMutex),
      m_dataCondition(dataCondition)
{
    qDebug() << "WzTcpServerThread()";
}

WzTcpServerThread::~WzTcpServerThread()
{
    qDebug() << "~WzTcpServerThread()";
}

void WzTcpServerThread::run()
{
    QTcpSocket tcpSocket;

    if (!tcpSocket.setSocketDescriptor(m_socketDescriptor)) {
        emit error(tcpSocket.error());
        return;
    }

    while (!m_abort) {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);
        bool hasData = false;
        {
            QMutexLocker lock(m_dataMutex);
            while (!m_abort) {
                hasData = m_dataCondition->wait(m_dataMutex, 100);
                if (hasData)
                    break;
            }
            if (hasData)
                out << *m_data;
        }

        if (hasData) {
            tcpSocket.write(block);
            qDebug() << "block.length:" << block.length();
            if (!tcpSocket.waitForBytesWritten(5000)) {
                qWarning() << "waitForBytesWritten error";
                break;
            }
        }
    }

    tcpSocket.disconnectFromHost();
    //tcpSocket->waitForDisconnected();
}

void WzTcpServerThread::abort()
{
    m_abort = true;
}
