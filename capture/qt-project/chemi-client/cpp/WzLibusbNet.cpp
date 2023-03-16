#include "WzLibusbNet.h"

WzLibusbNet::WzLibusbNet(QObject *parent) : QObject(parent)
{
    m_pCtlWs = new QWebSocket();
    QObject::connect(m_pCtlWs, &QWebSocket::stateChanged, this, &WzLibusbNet::controlWsStateChanged);
    QObject::connect(m_pCtlWs, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &WzLibusbNet::controlWsError);
    QObject::connect(m_pCtlWs, &QWebSocket::textMessageReceived, this, &WzLibusbNet::controlWsTextMessage);

    QObject::connect(&m_reconnectTimer, &QTimer::timeout, this, &WzLibusbNet::reconnectTimer);
    m_reconnectTimer.setSingleShot(false);
    m_reconnectTimer.setInterval(1000);
}

WzLibusbNet::~WzLibusbNet()
{
    m_reconnectTimer.stop();
    stop();
    delete m_pCtlWs;
}

void WzLibusbNet::exec(const QString &cmd)
{
    if (m_CtlWsState != QAbstractSocket::ConnectedState)
        return;
    m_pCtlWs->sendTextMessage(cmd);
}

void WzLibusbNet::start()
{
    qInfo() << "LibusbNet::start";
    if (m_serverAddress == "") {
        qWarning() << "\tServer address is nothing";
        return;
    }
    QString url = QString("ws://%1%2").arg(m_serverAddress, CONTROL_URI);
    qInfo() << "\tConnect " << url;
    m_pCtlWs->abort();
    m_pCtlWs->open(url);
}

void WzLibusbNet::stop()
{
    m_pCtlWs->close();
}

QString WzLibusbNet::serverAddress() const
{
    return m_serverAddress;
}

void WzLibusbNet::setServerAddress(const QString &serverAddress)
{
    m_serverAddress = serverAddress;
}

void WzLibusbNet::serverDisconnected()
{
    emit rsp("server_disconnected");
}

void WzLibusbNet::reconnectTimer()
{
    start();
}

void WzLibusbNet::controlWsStateChanged(QAbstractSocket::SocketState state)
{
    m_CtlWsState = state;
    if (state == QAbstractSocket::UnconnectedState) {
        serverDisconnected();
        m_reconnectTimer.stop();
        m_reconnectTimer.start();
    } else if (state == QAbstractSocket::ConnectedState) {
        m_reconnectTimer.stop();
    }
}

void WzLibusbNet::controlWsError(QAbstractSocket::SocketError error)
{
    qWarning() << "LibusbNet::Error" << error;
    if (error == QAbstractSocket::RemoteHostClosedError) {
        serverDisconnected();
        start();
    }
}

void WzLibusbNet::controlWsTextMessage(const QString &message)
{
    emit rsp(message);
}
