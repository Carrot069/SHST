#include "ShstHeartService.h"

ShstHeartService::ShstHeartService(QObject *parent)
    : ShstAbstractService{parent}
{
    qInfo() << "ShstHeartService";
}

ShstHeartService::~ShstHeartService()
{
    qInfo() << "~ShstHeartService";
}

bool ShstHeartService::init()
{
    return true;
}

bool ShstHeartService::uninit()
{
    return true;
}

bool ShstHeartService::processTextMessage(QWebSocket *webSocket, QString message)
{
    if (heart(webSocket, message))
        return true;
    if (getAliveCount(webSocket, message))
        return true;

    return false;
}

bool ShstHeartService::processBinaryMessage(QWebSocket *webSocket, QByteArray message)
{
    Q_UNUSED(webSocket)
    Q_UNUSED(message)
    return true;
}

bool ShstHeartService::heart(QWebSocket *webSocket, QString message)
{
    qInfo() << Q_FUNC_INFO;

    if (!isHeartMsg(message)) {
        //qInfo() << "\tIt's not a heart message";
        return false;
    }

    QString heartKey = getHeartKey(webSocket);
    updateHeart(heartKey);

    return true;
}

bool ShstHeartService::isHeartTimeout(const QDateTime &dt)
{
    return dt.msecsTo(QDateTime::currentDateTime()) >= TIMEOUT_MSEC;
}

bool ShstHeartService::getAliveCount(QWebSocket *webSocket, QString message)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if (!jsonDoc.isObject())
        return false;
    QJsonObject jsonObj = jsonDoc.object();
    if (!jsonObj.contains("getAliveCount"))
        return false;

    QMapIterator<QString, QDateTime> iter(m_alives);
    while (iter.hasNext()) {
        iter.next();
        if (isHeartTimeout(iter.value())) {
            m_alives.remove(iter.key());
        }
    }

    jsonObj["aliveCount"] = m_alives.count();
    webSocket->sendTextMessage(QJsonDocument(jsonObj).toJson());

    return true;
}

bool ShstHeartService::isHeartMsg(const QString &message)
{
    QJsonDocument json = QJsonDocument::fromJson(message.toUtf8());
    if (!json.isObject())
        return false;
    return json.object().contains("heart");
}

QString ShstHeartService::getHeartKey(const QWebSocket *webSocket)
{
    return QString("%1%2").arg(webSocket->peerAddress().toString()).arg(webSocket->peerPort());
}

void ShstHeartService::updateHeart(QString &key)
{
    m_alives[key] = QDateTime::currentDateTime();
}
