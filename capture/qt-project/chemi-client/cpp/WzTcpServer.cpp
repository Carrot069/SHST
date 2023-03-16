#include "WzTcpServer.h"

WzTcpServer::WzTcpServer(QObject *parent)
    : QTcpServer(parent)
{
}

WzTcpServer::~WzTcpServer()
{
    for (int i = 0; i < m_threads.count(); i++) {
        m_threads.at(i)->abort();
        m_threads.at(i)->wait();
    }
}

void WzTcpServer::writeData(QByteArray data)
{
    {
        QMutexLocker locker(&m_dataMutex);
        m_data = data;
    }
    m_dataCondition.wakeAll();
}

void WzTcpServer::incomingConnection(qintptr socketDescriptor)
{
    WzTcpServerThread *thread = new WzTcpServerThread(socketDescriptor, &m_data,
                                              &m_dataMutex, &m_dataCondition, nullptr);
    m_threads.append(thread);
    connect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));
    thread->start();
}

void WzTcpServer::threadFinished()
{
    for (int i = m_threads.count() - 1; i > -1; i--) {
        if (m_threads.at(i)->isFinished()) {
            delete m_threads.at(i);
            m_threads.removeAt(i);
        }
    }
}
