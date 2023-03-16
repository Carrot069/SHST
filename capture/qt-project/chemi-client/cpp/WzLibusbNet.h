#ifndef WZLIBUSBNET_H
#define WZLIBUSBNET_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>

class WzLibusbNet : public QObject
{
    Q_OBJECT

public:
    explicit WzLibusbNet(QObject *parent = nullptr);
    ~WzLibusbNet() override;

    Q_INVOKABLE void exec(const QString &cmd);
    void start(); // connect to server
    void stop(); // disconnect from server

    QString serverAddress() const;
    void setServerAddress(const QString &serverAddress);

    int serverPort() const;
    void setServerPort(int serverPort);

signals:
    void rsp(const QString& rsp);
    void execBefore(const QString &cmd);

private:
    const QString CONTROL_URI = "/mcu/control";

    QString m_serverAddress;

    QWebSocket *m_pCtlWs = nullptr; // Control WebSocket
    QAbstractSocket::SocketState m_CtlWsState;

    void serverDisconnected();

    QTimer m_reconnectTimer;
    void reconnectTimer();

private slots:
    void controlWsStateChanged(QAbstractSocket::SocketState state);
    void controlWsError(QAbstractSocket::SocketError error);
    void controlWsTextMessage(const QString &message);
};

#endif // WZLIBUSBNET_H
