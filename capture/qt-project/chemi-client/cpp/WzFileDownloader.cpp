#include "WzFileDownloader.h"

int WzFileDownloader::s_downloadID = 0;

WzFileDownloader::WzFileDownloader(QObject *parent) : QObject(parent)
{
    qDebug() << "WzFileDownloader";
    m_pTcpSocket = new QTcpSocket(this);
    m_tcpStream.setDevice(m_pTcpSocket);
    m_tcpStream.setVersion(QDataStream::Qt_5_12);
    QObject::connect(m_pTcpSocket, &QIODevice::readyRead, this, &WzFileDownloader::readData);
    QObject::connect(m_pTcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &WzFileDownloader::socketError);
    QObject::connect(m_pTcpSocket, &QAbstractSocket::stateChanged, this, &WzFileDownloader::socketStateChanged);
}

WzFileDownloader::~WzFileDownloader()
{
    m_pTcpSocket->abort();
    m_pTcpSocket->disconnectFromHost();
    delete m_pTcpSocket;
    qDebug() << "~WzFileDownloader";
}

int WzFileDownloader::downloadFile(const QStringList &remoteFiles, const QStringList &localFiles)
{
    if ("" == m_serverAddress) {
        qWarning("FileDownloader:downloadFile, serverAddress = """);
        return -1;
    }
    if (remoteFiles.count() != localFiles.count()) {
        qWarning("FileDownloader:downloadFile, localFiles.count != remoteFiles.count");
        return -1;
    }
    m_downloadID = s_downloadID;
    s_downloadID++;
    m_remoteFiles = remoteFiles;
    m_localFiles = localFiles;
    //m_pTcpSocket->abort();
    m_pTcpSocket->connectToHost(m_serverAddress, m_serverPort);
    qDebug() << "FileDownloader::downloadFile, " << m_serverAddress << m_serverPort;
    return m_downloadID;
}

QString WzFileDownloader::serverAddress() const
{
    return m_serverAddress;
}

void WzFileDownloader::setServerAddress(const QString &serverAddress)
{
    m_serverAddress = serverAddress;
}

int WzFileDownloader::serverPort() const
{
    return m_serverPort;
}

void WzFileDownloader::setServerPort(int serverPort)
{
    m_serverPort = serverPort;
}

int WzFileDownloader::downloadID() const
{
    return m_downloadID;
}

QStringList WzFileDownloader::remoteFiles() const
{
    return m_remoteFiles;
}

QStringList WzFileDownloader::localFiles() const
{
    return m_localFiles;
}

void WzFileDownloader::sendRequest(const QJsonObject &req)
{
    qDebug() << "FileDownloader::sendRequest";
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_12);
    stream << QJsonDocument(req).toJson();
    m_pTcpSocket->write(data);
}

void WzFileDownloader::sendDownloadRequest()
{
    qDebug() << "FileDownloader::sendDownloadRequest";
    m_tcpData.clear();
    QJsonArray fileNames;
    for (int i = 0; i < m_remoteFiles.count(); i++) {
        m_tcpData.append(QByteArray());
        fileNames.append(m_remoteFiles[i]);
    }
    QJsonObject req;
    req["fileNames"] = fileNames;
    sendRequest(req);
}

void WzFileDownloader::disconnect()
{
    qDebug() << "FileDownloader::disconnect";
    QJsonObject req;
    req["action"] = "disconnect";
    sendRequest(req);
}

void WzFileDownloader::readData()
{
    QByteArray ba;
    m_tcpStream.startTransaction();
    for (int i = 0; i < m_localFiles.count(); i++)
        m_tcpStream >> m_tcpData[i];
    if (!m_tcpStream.commitTransaction()) {
        qDebug() << "wait of tcp data";
        return;
    }
    qDebug() << "FileDownloader, tcp data received, m_tcpData.length:"
             << m_tcpData.length();

    this->disconnect();
    m_pTcpSocket->disconnectFromHost();

    for (int i = 0; i < m_localFiles.count(); i++) {

        QString localFile = m_localFiles[i];
        QFile f(localFile);
        if (!f.open(QFile::WriteOnly)) {
            emit error(m_downloadID, "cannot open file:" + localFile);
            return;
        }
        if (-1 == f.write(m_tcpData[i])) {
            emit error(m_downloadID, "file write error:" + localFile);
            return;
        }

        f.close();
        qDebug() << "FileDownloader::readData, file saved:" << localFile;

    }
    emit finished(m_downloadID);
}

void WzFileDownloader::socketError(QAbstractSocket::SocketError socketError)
{
    qWarning("FileDownloader::socketError, %d", socketError);
    emit error(m_downloadID,
               QString("socketError: %1").arg(socketError));
}

void WzFileDownloader::socketStateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << "FileDownloader::socketStateChanged:" << socketState;
    if (socketState == QAbstractSocket::ConnectedState) {
        sendDownloadRequest();
    }
}
