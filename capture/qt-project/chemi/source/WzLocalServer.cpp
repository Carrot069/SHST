#include "WzLocalServer.h"

WzLocalServer* g_localServer = nullptr;

WzLocalServer::WzLocalServer(QObject *parent) : QLocalServer(parent)
{
    bool ret = listen("SHSTCaptureLocalServer" + QString::number(QCoreApplication::applicationPid()));
    //bool ret = listen("test");

    if (!ret)
        qWarning() << "LocalServer, listen error";
    else
        qInfo() << "LocalServer, listen " << serverName();
}

void WzLocalServer::incomingConnection(quintptr socketDescriptor)
{
    qInfo() << "LocalServer::incomingConnection";
    auto localSocket = new QLocalSocket();
    if (!localSocket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "LocalServer, setSocketDescriptor error";
        return;
    }
    connect(localSocket, &QLocalSocket::disconnected, localSocket, &QLocalSocket::deleteLater);
    auto service = new WzLocalServerService(localSocket);
    connect(localSocket, &QLocalSocket::disconnected, service, &WzLocalServerService::deleteLater);
    connect(service, &WzLocalServerService::action, this, &WzLocalServer::action);
}
