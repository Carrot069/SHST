#ifndef WZFILEDOWNLOADER_H
#define WZFILEDOWNLOADER_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

class WzFileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit WzFileDownloader(QObject *parent = nullptr);
    ~WzFileDownloader() override;

    int downloadFile(const QStringList &remoteFiles, const QStringList &localFiles);

    QString serverAddress() const;
    void setServerAddress(const QString &serverAddress);

    int serverPort() const;
    void setServerPort(int serverPort);

    int downloadID() const;

    QStringList remoteFiles() const;

    QStringList localFiles() const;

signals:
    void finished(const int &id);
    void error(const int &id, const QString &error);

private:
    static int s_downloadID;

    int m_downloadID;
    QStringList m_remoteFiles;
    QStringList m_localFiles;

    QTcpSocket *m_pTcpSocket = nullptr;
    QDataStream m_tcpStream;
    QList<QByteArray> m_tcpData;

    QString m_serverAddress;
    int m_serverPort = 60054;

    void sendRequest(const QJsonObject &req);
    void sendDownloadRequest();
    void disconnect();

private slots:
    void readData();
    void socketError(QAbstractSocket::SocketError socketError);
    void socketStateChanged(QAbstractSocket::SocketState socketState);
};

#endif // WZFILEDOWNLOADER_H
