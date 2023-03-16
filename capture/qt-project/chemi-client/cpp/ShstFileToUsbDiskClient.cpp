#include "ShstFileToUsbDiskClient.h"

ShstFileToUsbDiskClient::ShstFileToUsbDiskClient(QObject *parent) : ShstTcpFileIo(parent)
{
    qInfo() << "ShstFileToUsbDiskClient";
    m_pTcpSocket = new QTcpSocket();

    QObject::connect(m_pTcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &ShstFileToUsbDiskClient::socketError);
    QObject::connect(m_pTcpSocket, &QAbstractSocket::stateChanged, this, &ShstFileToUsbDiskClient::socketStateChanged);
    QObject::connect(m_pTcpSocket, &QAbstractSocket::connected, this, &ShstFileToUsbDiskClient::socketConnected);
    QObject::connect(m_pTcpSocket, &QAbstractSocket::disconnected, this, &ShstFileToUsbDiskClient::socketDisconnected);
    QObject::connect(m_pTcpSocket, &QAbstractSocket::bytesWritten, this, &ShstFileToUsbDiskClient::socketBytesWritten);

    setTcpSocket(m_pTcpSocket);
}

ShstFileToUsbDiskClient::~ShstFileToUsbDiskClient()
{
    m_pTcpSocket->abort();
    m_pTcpSocket->disconnectFromHost();
    delete m_pTcpSocket;
    qDebug() << "~ShstFileToUsbDiskClient";
}

void ShstFileToUsbDiskClient::sendFiles(const QStringList &remoteFiles, const QStringList &localFiles)
{
    qInfo() << "ShstFileToUsbDiskClient::sendFiles";

    if ("" == m_serverAddress) {
        qWarning("\tThe serverAddress is nothing");
        return;
    }
    if (m_serverPort == 0) {
        qWarning("\tThe serverPort is zero");
        return;
    }

    m_remoteFiles = remoteFiles;
    m_localFiles = localFiles;

    qInfo() << "\tremoteFiles:" << remoteFiles;
    qInfo() << "\tlocalFiles:" << localFiles;

    if (m_pTcpSocket->state() == QAbstractSocket::UnconnectedState) {
        qInfo() << "\tConnect to" << m_serverAddress + ":" + QString::number(m_serverPort);
        m_isWaitSendFiles = true;
        m_pTcpSocket->connectToHost(m_serverAddress, static_cast<quint16>(m_serverPort));
    } else if (m_pTcpSocket->state() == QAbstractSocket::ConnectedState) {
        ShstTcpFileIo::sendFiles(m_localFiles, m_remoteFiles);
    }
}

QString ShstFileToUsbDiskClient::serverAddress() const
{
    return m_serverAddress;
}

void ShstFileToUsbDiskClient::setServerAddress(const QString &serverAddress)
{
    m_serverAddress = serverAddress;
}

int ShstFileToUsbDiskClient::serverPort() const
{
    return m_serverPort;
}

void ShstFileToUsbDiskClient::setServerPort(int serverPort)
{
    m_serverPort = serverPort;
}

bool ShstFileToUsbDiskClient::isConnected() const
{
    return m_isConnected;
}

QStringList ShstFileToUsbDiskClient::remoteFiles() const
{
    return m_remoteFiles;
}

QStringList ShstFileToUsbDiskClient::localFiles() const
{
    return m_localFiles;
}

void ShstFileToUsbDiskClient::readyJson()
{
    ShstTcpFileIo::readyJson();
    QString action = m_receivedJsonDocument["action"].toString();
    if (action == "finished") {
        sendJson({{"action", "disconnect"}}, WaitJson);
        emit finished();
        m_pTcpSocket->flush();
        m_pTcpSocket->disconnectFromHost();
    }
}

void ShstFileToUsbDiskClient::readyFile(const QString fileName, const QByteArray &file)
{
    qInfo() << "ShstFileToUsbDiskClient::readyFile";
    qInfo() << "\t" << fileName;
    qInfo() << "\tSize:" << file.count();
}

void ShstFileToUsbDiskClient::socketError(QAbstractSocket::SocketError socketError)
{

    qInfo() << "ShstFileToUsbDiskClient::socketError";
    qWarning() << "\t" << socketError << m_pTcpSocket->errorString();
    emit ShstTcpIo::error(m_pTcpSocket->errorString());
}

void ShstFileToUsbDiskClient::socketStateChanged(QAbstractSocket::SocketState socketState)
{
    qInfo() << "ShstFileToUsbDiskClient::socketStateChanged";
    qInfo() << "\tState:" << socketState;
}

void ShstFileToUsbDiskClient::socketConnected()
{
    qInfo() << "ShstFileToUsbDiskClient::socketConnected";
    qInfo() << "\tThread Id:" << QThread::currentThreadId();
    m_isConnected = true;
    if (m_isWaitSendFiles) {
        m_isWaitSendFiles = false;
        sendFiles(m_remoteFiles, m_localFiles);
    }
}

void ShstFileToUsbDiskClient::socketDisconnected()
{
    m_isConnected = false;
}

void ShstFileToUsbDiskClient::socketBytesWritten(qint64 bytes)
{
    qInfo() << "ShstFileToUsbDiskClient::socketBytesWritten: " << bytes;
}
