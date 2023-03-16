#include "ShstServer.h"

ShstServer::ShstServer(QObject *parent)
    : QObject(parent)
{
    qInfo() << "ShstServer";
}

ShstServer::~ShstServer()
{
    qInfo() << "~ShstServer";
}

bool ShstServer::init()
{
    qInfo() << "ShstServer::init";

    if (!m_webSocketServer)
        m_webSocketServer = new QWebSocketServer(QStringLiteral("SHST Server"), QWebSocketServer::NonSecureMode, this);

    if (!m_webSocketServer->isListening()) {
        if (m_webSocketServer->listen()) {
            connect(m_webSocketServer, &QWebSocketServer::newConnection,
                             this, &ShstServer::onNewConnection);
            WzUdpBroadcastSender::ServerPort = m_webSocketServer->serverPort();
            qInfo() << "\tlistening on port" << m_webSocketServer->serverPort();
        } else {
            qInfo() << "\tlisten failure";
            return false;
        }

        m_services.append(new ShstUsbDiskService());
        m_services.last()->init();
        m_services.append(new ShstHeartService());
        m_services.last()->init();
    }

    return true;
}

bool ShstServer::uninit()
{
    foreach(ShstAbstractService* s, m_services) {
        m_services.removeAll(s);
        s->uninit();
        delete s;
    }

    if (m_webSocketServer) {
        m_webSocketServer->close();
        delete m_webSocketServer;
        m_webSocketServer = nullptr;
    }

    return true;
}

quint16 ShstServer::port() const
{
    if (m_webSocketServer) {
        return m_webSocketServer->serverPort();
    } else {
        return 0;
    }
}

void ShstServer::onNewConnection()
{
    qInfo() << "ShstServer::onNewConnection";

    auto pSocket = m_webSocketServer->nextPendingConnection();
    qInfo() << "\t" << pSocket->peerAddress() << pSocket->peerPort();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &ShstServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &ShstServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &ShstServer::socketDisconnected);

    m_clients << pSocket;
}

void ShstServer::socketDisconnected()
{
    qInfo() << "ShstServer::socketDisconnected";

    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    qInfo() << "\tsocketDisconnected:" << pClient->peerAddress() << pClient->peerPort();
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

void ShstServer::processTextMessage(QString message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    foreach (ShstAbstractService* s, m_services) {
        if (s->processTextMessage(pClient, message)) {
            break;
        }
    }
}

void ShstServer::processBinaryMessage(QByteArray message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    foreach (ShstAbstractService* s, m_services) {
        if (s->processBinaryMessage(pClient, message)) {
            break;
        }
    }
}
