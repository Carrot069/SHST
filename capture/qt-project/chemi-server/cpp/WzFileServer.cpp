#include "WzFileServer.h"

WzFileServer::WzFileServer(QObject *parent) : QTcpServer(parent)
{

}

WzFileServer::~WzFileServer()
{
    for (int i = 0; i < m_services.count(); i++) {
        delete m_services.at(i);
    }
}

void WzFileServer::incomingConnection(qintptr socketDescriptor)
{
    qInfo() << "FileServer::incomingConnection";
    WzFileService *service = new WzFileService();
    if (service->init(socketDescriptor)) {
        m_services.append(service);
        connect(service, SIGNAL(finished()), this, SLOT(serviceFinished()));
    } else {
        qWarning() << "\tFileService init failure";
        delete service;
    }
}

void WzFileServer::serviceFinished()
{
    for (int i = m_services.count() - 1; i > -1; i--) {
        if (m_services.at(i)->isFinished()) {
            m_services.at(i)->deleteLater();
            m_services.removeAt(i);
        }
    }
}
