#include "ShstFileServer.h"

ShstFileServer::ShstFileServer(QObject *parent) : QTcpServer(parent)
{
    qInfo() << "ShstFileServer";
}

ShstFileServer::~ShstFileServer()
{
    for (int i = 0; i < m_services.count(); i++) {
        delete m_services.at(i);
    }
    qInfo() << "~ShstFileServer";
}

void ShstFileServer::incomingConnection(qintptr socketDescriptor)
{
    qInfo() << "ShstFileServer::incomingConnection";
    ShstTcpFileService *service = new ShstTcpFileService();
    if (service->init(socketDescriptor)) {
        m_services.append(service);
        connect(service, SIGNAL(finished()), this, SLOT(serviceFinished()));
    } else {
        qWarning() << "\tFileService init failure";
        delete service;
    }
}

void ShstFileServer::serviceFinished()
{
    for (int i = m_services.count() - 1; i > -1; i--) {
        if (m_services.at(i)->isFinished()) {
            m_services.at(i)->deleteLater();
            m_services.removeAt(i);
        }
    }
}
