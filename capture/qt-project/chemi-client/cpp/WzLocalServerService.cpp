#include "WzLocalServerService.h"

WzLocalServerService::WzLocalServerService(QLocalSocket *tcpSocket,
                                           QObject *parent)
    : QObject(parent), m_pLocalSocket(tcpSocket)
{
    qDebug() << "WzLocalServerService";
    QObject::connect(m_pLocalSocket, &QIODevice::readyRead, this, &WzLocalServerService::readyRead);
    QObject::connect(m_pLocalSocket, &QLocalSocket::stateChanged, this, &WzLocalServerService::stateChanged);
    m_inStream.setDevice(m_pLocalSocket);
    m_inStream.setVersion(QDataStream::Qt_5_12);
}

WzLocalServerService::~WzLocalServerService()
{
    qDebug() << "~WzLocalServerService";
}

void WzLocalServerService::readyRead()
{
    qDebug() << "LocalServerService::readyRead";

    QByteArray request;

    m_inStream.startTransaction();
    m_inStream >> request;
    if (!m_inStream.commitTransaction())
        return;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(request);
    auto action = jsonDoc["action"].toString();
    if (action == "disconnect") {
        m_pLocalSocket->disconnectFromServer();
        return;
    }

    emit this->action(jsonDoc);
}

void WzLocalServerService::stateChanged(QLocalSocket::LocalSocketState socketState)
{
    qDebug() << "LocalServerService::stateChanged, " << socketState;
}
