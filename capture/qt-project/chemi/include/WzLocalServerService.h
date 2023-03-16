#ifndef WZFILESERVERTHREAD_H
#define WZFILESERVERTHREAD_H

#include <QThread>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <QFile>
#include <QMutex>
#include <QWaitCondition>

class WzLocalServerService : public QObject
{
    Q_OBJECT

public:
    WzLocalServerService(QLocalSocket *tcpSocket, QObject *parent = nullptr);
    ~WzLocalServerService() override;

    bool isFinished() const;

signals:
    void action(const QJsonDocument &jsonDocument);

private slots:
    void readyRead();
    void stateChanged(QLocalSocket::LocalSocketState socketState);

private:
    QDataStream m_inStream;
    QLocalSocket *m_pLocalSocket;

    void sendFiles(const QJsonDocument &jsonDoc);
};

#endif // WZFILESERVERTHREAD_H
