#ifndef WZFILESERVERTHREAD_H
#define WZFILESERVERTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <QFile>
#include <QMutex>
#include <QWaitCondition>

/*
 * 每个新连接会创建一个对象, 读取来自客户端的json请求, 请求中包含了
 * 需要下载的文件的名称列表。
 */
class WzFileService : public QObject
{
    Q_OBJECT

public:
    WzFileService();
    ~WzFileService() override;

    bool init(qintptr socketDescriptor);
    bool isFinished() const;

signals:
    void error(QTcpSocket::SocketError socketError);
    void finished();

private slots:
    void readyRead();
    void stateChanged(QAbstractSocket::SocketState socketState);

private:
    QDataStream m_inStream;
    QTcpSocket *m_pTcpSocket = nullptr;
    bool m_isFinished = false;

    void sendFiles(const QJsonDocument &jsonDoc);
};

#endif // WZFILESERVERTHREAD_H
