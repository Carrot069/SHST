#ifndef SHSTFILETOUSBDISKCLIENT_H
#define SHSTFILETOUSBDISKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QThread>

#include "ShstTcpFileIo.h"

class ShstFileToUsbDiskClient : public ShstTcpFileIo
{
    Q_OBJECT
public:
    explicit ShstFileToUsbDiskClient(QObject *parent = nullptr);
    ~ShstFileToUsbDiskClient() override;

    void sendFiles(const QStringList &remoteFiles, const QStringList &localFiles);

    QString serverAddress() const;
    void setServerAddress(const QString &serverAddress);
    int serverPort() const;
    void setServerPort(int serverPort);
    bool isConnected() const;

    QStringList remoteFiles() const;
    QStringList localFiles() const;

signals:
    void finished();

protected:
    void readyJson() override;
    void readyFile(const QString fileName, const QByteArray &file) override;

private:
    QStringList m_remoteFiles;
    QStringList m_localFiles;
    QTcpSocket *m_pTcpSocket = nullptr;
    bool m_isWaitSendFiles = false;

    QString m_serverAddress;
    int m_serverPort = 60054;
    bool m_isConnected = false;

private slots:
    void socketError(QAbstractSocket::SocketError socketError);
    void socketStateChanged(QAbstractSocket::SocketState socketState);
    void socketConnected();
    void socketDisconnected();
    void socketBytesWritten(qint64 bytes);
};

#endif // SHSTFILETOUSBDISKCLIENT_H
