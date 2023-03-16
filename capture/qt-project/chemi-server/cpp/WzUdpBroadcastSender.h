#ifndef WZUDPBROADCASTSENDER_H
#define WZUDPBROADCASTSENDER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QNetworkInterface>
#include <QHostAddress>

class WzUdpBroadcastSender : public QObject
{
    Q_OBJECT
public:
    static int PreviewPort;
    static int CameraPort;
    static int McuPort;
    static int FilePort;
    static int FilePort2;
    // 2021-12-28 15:08:27 added by wz
    // 通用消息端口, 逐步将其他端口的服务转移到此端口
    static int ServerPort;

    explicit WzUdpBroadcastSender(const int port, QObject *parent = nullptr);

private slots:
    void broadcastDatagram();

private:
    QUdpSocket *m_udpSocket = nullptr;
    QTimer m_timer;
    int m_port = 0;

    QList<QHostAddress> m_allBroadcastAddresses;
    QList<QHostAddress> getAllBroadcastAddresses();
};

#endif // WZUDPBROADCASTSENDER_H
