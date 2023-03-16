#include "WzUdpBroadcastSender.h"

int WzUdpBroadcastSender::PreviewPort = 0;
int WzUdpBroadcastSender::CameraPort = 0;
int WzUdpBroadcastSender::McuPort = 0;
int WzUdpBroadcastSender::FilePort = 0;
int WzUdpBroadcastSender::FilePort2 = 0;
int WzUdpBroadcastSender::ServerPort = 0;

WzUdpBroadcastSender::WzUdpBroadcastSender(const int port, QObject *parent) : QObject(parent)
{
    qInfo() << Q_FUNC_INFO;
    qInfo() << "\tport" << port;
    m_allBroadcastAddresses = getAllBroadcastAddresses();
    m_port = port;
    m_udpSocket = new QUdpSocket(this);
    connect(&m_timer, &QTimer::timeout, this, &WzUdpBroadcastSender::broadcastDatagram);
    m_timer.setInterval(1000);
    m_timer.setSingleShot(false);
    m_timer.start();
}

void WzUdpBroadcastSender::broadcastDatagram()
{
    qInfo() << Q_FUNC_INFO;
    if (PreviewPort == 0) return;
    if (CameraPort == 0) return;
    if (McuPort == 0) return;
    if (FilePort == 0) return;
    if (FilePort2 == 0) return;
    if (ServerPort == 0) return;

    QJsonObject json;
    json["serverHostName"] = QHostInfo::localHostName();
    json["previewPort"] = PreviewPort;
    json["cameraPort"] = CameraPort;
    json["mcuPort"] = McuPort;
    json["filePort"] = FilePort;
    json["filePort2"] = FilePort2;
    json["serverPort"] = ServerPort;
    QByteArray datagram = QJsonDocument(json).toBinaryData();
    foreach(auto address, m_allBroadcastAddresses)
        m_udpSocket->writeDatagram(datagram, address, static_cast<quint16>(m_port));
}

QList<QHostAddress> WzUdpBroadcastSender::getAllBroadcastAddresses()
{
    qInfo() << Q_FUNC_INFO;
    QList<QHostAddress> addresses;
    auto allInterfaces = QNetworkInterface::allInterfaces();
    foreach (auto interface, allInterfaces) {
        if (interface.flags().testFlag(QNetworkInterface::CanBroadcast) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning)) {
            foreach (auto ae, interface.addressEntries()) {
                if (!ae.broadcast().isNull()) {
                    addresses << ae.broadcast();
                    qInfo() << "\t" << ae.broadcast();
                }
            }
        }
    }
    return addresses;
}
