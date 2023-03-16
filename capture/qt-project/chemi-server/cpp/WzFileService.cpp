#include "WzFileService.h"

WzFileService::WzFileService()
    : QObject(nullptr)
{
    qInfo() << "WzFileService";
    m_pTcpSocket = new QTcpSocket();
}

WzFileService::~WzFileService()
{
    m_pTcpSocket->disconnectFromHost();
    m_pTcpSocket->deleteLater();
    qInfo() << "~WzFileService";
}

bool WzFileService::init(qintptr socketDescriptor)
{
    qInfo() << "FileService::init";
    if (!m_pTcpSocket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "\tsetSocketDescriptor error:" << m_pTcpSocket->error() << ","
                   << m_pTcpSocket->errorString();
        return false;
    }
    QObject::connect(m_pTcpSocket, &QIODevice::readyRead, this, &WzFileService::readyRead);
    QObject::connect(m_pTcpSocket, &QAbstractSocket::stateChanged, this, &WzFileService::stateChanged);
    m_inStream.setDevice(m_pTcpSocket);
    m_inStream.setVersion(QDataStream::Qt_5_12);
    return true;
}

void WzFileService::readyRead()
{
    qInfo() << "FileService::readyRead";

    QByteArray request;

    m_inStream.startTransaction();
    m_inStream >> request;
    if (!m_inStream.commitTransaction())
        return;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(request);
    auto action = jsonDoc["action"].toString();
    if (action == "disconnect") {
        m_pTcpSocket->disconnectFromHost();
        m_isFinished = true;
        emit finished();
        return;
    }

    sendFiles(jsonDoc);
}

void WzFileService::stateChanged(QAbstractSocket::SocketState socketState)
{
    qInfo() << "FileService::stateChanged, " << socketState;
    if (socketState == QAbstractSocket::ClosingState) {
        m_isFinished = true;
        emit finished();
    }
}

bool WzFileService::isFinished() const
{
    return m_isFinished;
}

void WzFileService::sendFiles(const QJsonDocument &jsonDoc)
{
    auto fileNames = jsonDoc["fileNames"].toArray();

    QByteArray outData;
    QDataStream outStream(&outData, QIODevice::WriteOnly);
    outStream.setVersion(QDataStream::Qt_5_12);

    for (int i = 0; i < fileNames.count(); i++) {

        QFile f(fileNames[i].toString());
        if (!f.exists()) {
            qWarning() << "FileService, file not found:" << f.fileName();
            continue;
        } else if (!f.open(QFile::ReadOnly)) {
            qWarning() << "FileService, cannot open file:" << f.fileName();
            continue;
        }
        qInfo() << "read file: " << f.fileName();
        outStream << f.readAll();
        f.close();
        f.remove();
    }

    m_pTcpSocket->write(outData);
    m_pTcpSocket->flush();
}


