#include "WzUdpBroadcastReceiver.h"

QString WzUdpBroadcastReceiver::ServerHost = "";
int WzUdpBroadcastReceiver::PreviewPort = 0;
int WzUdpBroadcastReceiver::CameraPort = 0;
int WzUdpBroadcastReceiver::McuPort = 0;
int WzUdpBroadcastReceiver::FilePort = 0;
int WzUdpBroadcastReceiver::FilePort2 = 0;
int WzUdpBroadcastReceiver::ServerPort = 0;

WzUdpBroadcastReceiver::WzUdpBroadcastReceiver(
        const int port, QObject *parent) : QObject(parent)
{

    m_udpSocket = new QUdpSocket(this);
    m_udpSocket->bind(static_cast<quint16>(port), QUdpSocket::ShareAddress);

    connect(m_udpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()));

}

void WzUdpBroadcastReceiver::processPendingDatagrams()
{
    if (m_udpMsgCount % 20 == 0)
        qInfo() << "UdpBroadcastReceiver::processPendingDatagrams";
    m_udpMsgCount++;
    QByteArray datagram;
    while (m_udpSocket->hasPendingDatagrams()) {
        //qInfo() << "UdpBroadcastReceiver::processPendingDatagrams, size:"
        //        << int(m_udpSocket->pendingDatagramSize());
        datagram.resize(int(m_udpSocket->pendingDatagramSize()));
        QHostAddress udpSenderAddress;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &udpSenderAddress);
        QHostAddress udpSenderAddressIPv4(udpSenderAddress.toIPv4Address());
        QJsonDocument jsonDoc = QJsonDocument::fromBinaryData(datagram);
        if (jsonDoc.isObject()) {
            if (m_serverHostName != udpSenderAddressIPv4.toString()) {                                
                m_serverHostName = udpSenderAddressIPv4.toString();
                ServerHost = m_serverHostName;
                PreviewPort = jsonDoc["previewPort"].toInt();
                CameraPort = jsonDoc["cameraPort"].toInt();
                McuPort = jsonDoc["mcuPort"].toInt();
                FilePort = jsonDoc["filePort"].toInt();
                FilePort2 = jsonDoc["filePort2"].toInt();
                ServerPort = jsonDoc["serverPort"].toInt();
                emit serverHostNameChanged(m_serverHostName);
                qInfo() << QString("PreviewPort:%1, CameraPort:%2, McuPort:%3, FilePort:%4, ServerPort:%5, FilePort2:%6")
                           .arg(PreviewPort)
                           .arg(CameraPort)
                           .arg(McuPort)
                           .arg(FilePort)
                           .arg(ServerPort)
                           .arg(FilePort2);
            }
        }
    }
}
