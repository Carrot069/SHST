#include "ShstUsbDiskService.h"

ShstUsbDiskService::ShstUsbDiskService(QObject *parent)
    : ShstAbstractService{parent}
{
    qInfo() << "ShstUsbDiskService";
}

ShstUsbDiskService::~ShstUsbDiskService()
{
    qInfo() << "~ShstUsbDiskService";
}

bool ShstUsbDiskService::init()
{
    return true;
}

bool ShstUsbDiskService::uninit()
{
    return true;
}

bool ShstUsbDiskService::processTextMessage(QWebSocket *webSocket, QString message)
{
    qInfo() << "ShstUsbDiskService::processTextMessage";

    QJsonParseError error;
    auto jsonDoc = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (jsonDoc.isNull()) {
        qWarning() << error.errorString();
        return false;
    }

    QString action = jsonDoc["action"].toString();
    QJsonObject response;
    if (getUsbDiskCount(action, response)) {
        sendJsonResponse(webSocket, response);
        return true;
    }

    return false;
}

bool ShstUsbDiskService::processBinaryMessage(QWebSocket *webSocket, QByteArray message)
{
    Q_UNUSED(webSocket)
    Q_UNUSED(message)
    return true;
}

bool ShstUsbDiskService::sendJsonResponse(QWebSocket *ws, const QJsonObject &response)
{
    qInfo() << "ShstUsbDiskService::sendJsonResponse";
    if (ws) {
        auto bytes = QJsonDocument(response).toJson(QJsonDocument::Compact);
        ws->sendTextMessage(QString(bytes));
        return true;
    } else {
        return false;
    }
}

bool ShstUsbDiskService::getUsbDiskCount(const QString &action, QJsonObject &response) const
{
    if (action != "getUsbDiskCount")
        return false;
    ShstUsbDisk ud;
    QStringList drives;
    auto count = ud.getCount(&drives);
    response["action"] = action;
    response["count"] = count;
    response["drives"] = QJsonArray::fromStringList(drives);

    return true;
}
