#include "ShstTcpFileService.h"

ShstTcpFileService::ShstTcpFileService(QObject *parent)
    : ShstTcpFileIo(parent)
{
    qInfo() << "ShstTcpFileService";
    m_pTcpSocket = new QTcpSocket();
}

ShstTcpFileService::~ShstTcpFileService()
{
    m_pTcpSocket->disconnectFromHost();
    m_pTcpSocket->deleteLater();
    qInfo() << "~TcpFileService";
}

bool ShstTcpFileService::init(qintptr socketDescriptor)
{
    qInfo() << "ShstTcpFileService::init";
    QObject::connect(m_pTcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &ShstTcpFileService::socketError);
    QObject::connect(m_pTcpSocket, &QAbstractSocket::stateChanged, this, &ShstTcpFileService::socketStateChanged);
    setTcpSocket(m_pTcpSocket);
    if (!m_pTcpSocket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "\tsetSocketDescriptor error:" << m_pTcpSocket->error() << ","
                   << m_pTcpSocket->errorString();
        return false;
    }
    waitJson();
    return true;
}

void ShstTcpFileService::socketStateChanged(QAbstractSocket::SocketState socketState)
{
    qInfo() << "ShstTcpFileService::socketStateChanged";
    qInfo() << "\tState:" << socketState;
}

bool ShstTcpFileService::isFinished() const
{
    return m_isFinished;
}

void ShstTcpFileService::readyFile(const QString fileName, const QByteArray &file)
{
    qInfo() << "ShstTcpFileService::readyFile";
    qInfo() << "\tFileName:" << fileName;
    qInfo() << "\tFileSize:" << file.size();

    QFileInfo fi(fileName);
    QDir dir = fi.absoluteDir();
    dir.mkpath(".");
    QFile f(fileName);
    if (!f.open(QFile::WriteOnly)) {
        qWarning() << "\tOpen file failure";
        return;
    }
    f.write(file);
    f.close();
}

void ShstTcpFileService::socketError(QAbstractSocket::SocketError socketError)
{
    qWarning() << "ShstTcpFileService::socketError";
    qWarning() << "\tError:" << socketError << "," << m_pTcpSocket->errorString();
}
