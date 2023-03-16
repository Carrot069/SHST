#include "ShstTcpIo.h"

ShstTcpIo::ShstTcpIo(QObject *parent): QObject(parent)
{
    qInfo() << "[TcpIo]";
}

ShstTcpIo::~ShstTcpIo()
{
    qInfo() << "[~TcpIo]";
}

bool ShstTcpIo::sendJson(const QJsonObject &json, const WaitDataType &waitDataType)
{
    qInfo() << "[TcpIo]sendJson";

    this->m_waitDataType = waitDataType;

    if (waitDataType == WaitJson)
        waitJson();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_12);
    stream << QJsonDocument(json).toJson(QJsonDocument::Compact);
    m_tcpSocket->write(data);

    return true;
}

void ShstTcpIo::waitData(const int count)
{
    m_receivedData.clear();
    for (int i = 0; i < count; i++) {
        m_receivedData.append(QByteArray());
    }
}

void ShstTcpIo::waitJson()
{
    m_waitDataType = WaitJson;
    waitData(1);
}

void ShstTcpIo::waitStream(const int count)
{
    m_waitDataType = WaitStream;
    waitData(count);
}

ShstTcpIo::WaitDataType ShstTcpIo::waitDataType() const
{
    return m_waitDataType;
}

void ShstTcpIo::setWaitDataType(WaitDataType newWaitDataType)
{
    m_waitDataType = newWaitDataType;
}

QTcpSocket *ShstTcpIo::tcpSocket() const
{
    return m_tcpSocket;
}

void ShstTcpIo::setTcpSocket(QTcpSocket *newTcpSocket)
{
    m_tcpSocket = newTcpSocket;
    m_receivedStream.setDevice(m_tcpSocket);
    m_receivedStream.setVersion(QDataStream::Qt_5_12);
    QObject::connect(m_tcpSocket, &QIODevice::readyRead, this, &ShstTcpIo::readyRead);
}

bool ShstTcpIo::parseJson()
{
    qInfo() << "[TcpIo]parseJson";
    QJsonParseError err;
    m_receivedJsonDocument = QJsonDocument::fromJson(m_receivedData.first(), &err);
    if (m_receivedJsonDocument.isNull()) {
        emit error("\t" + err.errorString());
        return false;
    }
    return true;
}

void ShstTcpIo::readyRead()
{
    m_receivedStream.startTransaction();

    if (m_waitDataType == WaitJson) {
        m_receivedStream >> m_receivedData.first();
    } else if (m_waitDataType == WaitStream) {
        for (int i = 0; i < m_receivedData.count(); i++)
            m_receivedStream >> m_receivedData[i];
    }
    if (!m_receivedStream.commitTransaction()) {
        //qInfo() << "\tWait for the tcp data";
        return;
    }
    qInfo() << "\tThe tcp data received, length:";
    for (int i = 0; i < m_receivedData.count(); i++)
        qInfo() << "\t" << m_receivedData[i].size();

    if (m_waitDataType == WaitJson) {
        if (parseJson())
            readyJson();
    } else if (m_waitDataType == WaitStream) {
        readyStream();
    }
}
