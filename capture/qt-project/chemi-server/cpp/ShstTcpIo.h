#ifndef SHSTTCPIO_H
#define SHSTTCPIO_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>

/* Author: wangzhe
 * Time: 2021-12-22 11:28:44
 * 使用已建立好的TCP连接进行JSON/二进制双向传递
 * 本类设计目的是作为基类
**/
class ShstTcpIo : public QObject
{
    Q_OBJECT

public:
    enum WaitDataType {
        WaitJson,
        WaitStream
    };

    explicit ShstTcpIo(QObject *parent = nullptr);
    ~ShstTcpIo() override;

    QTcpSocket *tcpSocket() const;
    void setTcpSocket(QTcpSocket *newTcpSocket);

signals:
    void error(const QString error);

protected:
    QJsonDocument m_receivedJsonDocument;
    QDataStream m_receivedStream;
    QList<QByteArray> m_receivedData;

    bool sendJson(const QJsonObject &json, const WaitDataType &waitDataType);
    void waitData(const int count);
    void waitJson();
    void waitStream(const int count);
    virtual void readyJson() = 0;
    virtual void readyStream() = 0;

    WaitDataType waitDataType() const;
    void setWaitDataType(WaitDataType newWaitDataType);
private:
    QTcpSocket *m_tcpSocket = nullptr;    
    WaitDataType m_waitDataType;

    bool parseJson();
private slots:
    void readyRead();
};

#endif // SHSTTCPIO_H
