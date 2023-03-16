#ifndef WZUDPBROADCASTRECEIVER_H
#define WZUDPBROADCASTRECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include <QJsonObject>
#include <QJsonDocument>

class WzUdpBroadcastReceiver : public QObject
{
    Q_OBJECT
public:
    static QString ServerHost;
    static int PreviewPort;
    static int CameraPort;
    static int McuPort;
    static int FilePort;
    static int FilePort2;
    static int ServerPort;

    explicit WzUdpBroadcastReceiver(const int port, QObject *parent = nullptr);

private slots:
    void processPendingDatagrams();

private:
    QUdpSocket *m_udpSocket = nullptr;
    QString m_serverHostName = "";
    int m_udpMsgCount = 0;

signals:
    void serverHostNameChanged(const QString &hostName);
};

#endif // WZUDPBROADCASTRECEIVER_H
