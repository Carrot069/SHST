#ifndef SHSTSERVER_H
#define SHSTSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>

#include "WzUdpBroadcastSender.h"

#include "ShstAbstractService.h"
#include "service/ShstUsbDiskService.h"
#include "service/ShstHeartService.h"

class ShstServer : public QObject
{
    Q_OBJECT
public:
    explicit ShstServer(QObject *parent = nullptr);
    ~ShstServer() override;

    bool init();
    bool uninit();
    quint16 port() const;

private:
    QWebSocketServer* m_webSocketServer = nullptr;
    QList<QWebSocket*> m_clients;
    QList<ShstAbstractService*> m_services;

private slots:
    void onNewConnection();
    void socketDisconnected();
    void processTextMessage(QString message);
    void processBinaryMessage(QByteArray message);

signals:

};

#endif // SHSTSERVER_H
