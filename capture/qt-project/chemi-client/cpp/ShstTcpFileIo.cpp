#include "ShstTcpFileIo.h"

ShstTcpFileIo::ShstTcpFileIo(QObject *parent)
    : ShstTcpIo(parent)
{

}

void ShstTcpFileIo::sendFiles(const QStringList &fileNames, const QStringList &remoteFileNames)
{
    qInfo() << "[TcpFileIo]sendFiles";

    QJsonObject req;
    req["action"] = "sendFiles";
    QJsonArray remoteFileNamesArray;
    for (int i = 0; i < fileNames.count(); i++)
        remoteFileNamesArray.append(remoteFileNames[i]);
    req["fileNames"] = remoteFileNamesArray;

    sendJson(req, WaitJson);

    QByteArray outData;
    QDataStream outStream(&outData, QIODevice::WriteOnly);
    outStream.setVersion(QDataStream::Qt_5_12);

    for (int i = 0; i < fileNames.count(); i++) {
        QFile f(fileNames[i]);
        if (!f.exists()) {
            qWarning() << "\tFile not found:" << f.fileName();
            continue;
        } else if (!f.open(QFile::ReadOnly)) {
            qWarning() << "\tFile cannot open:" << f.fileName();
            continue;
        }
        qInfo() << "\tRead file: " << f.fileName();
        outStream << f.readAll();
        f.close();
        readyFile(fileNames[i], QByteArray());
    }

    m_elapsedTimer.start();
    tcpSocket()->write(outData);
    tcpSocket()->flush();

    waitJson();
}

void ShstTcpFileIo::readyJson()
{
    qInfo() << "[TcpFileIo]readyJson";
    auto action = m_receivedJsonDocument["action"].toString();
    qInfo() << "\tThe action is" << action;
    if (action == "sendFiles") {
        auto fileNames = m_receivedJsonDocument["fileNames"].toArray();
        for (int i = 0; i < fileNames.count(); i++)
            m_fileNames << fileNames[i].toString();
        waitStream(m_fileNames.count());
    } else if (action == "finished") {
        qInfo() << "\tFinished:"
                << m_elapsedTimer.elapsed()
                << " msec";
    } else if (action == "disconnect") {
        tcpSocket()->disconnectFromHost();
    }
}

void ShstTcpFileIo::readyStream()
{
    qInfo() << "[TcpFileIo]readyStream";
    qInfo() << "\tReceived data count:" << m_receivedData.count()
            << ", FileNames count:" << m_fileNames.count();
    for (int i = 0; i < m_receivedData.count(); i++) {
        readyFile(m_fileNames[i], m_receivedData[i]);
    }
    sendJson({{"action", "finished"}}, WaitJson);
}

ShstTcpFileIo::~ShstTcpFileIo()
{
    qInfo() << "[~TcpFileIo]";
}
