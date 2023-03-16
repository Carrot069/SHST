#ifndef SHSTUSBDISKSERVICE_H
#define SHSTUSBDISKSERVICE_H

#include <QObject>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../ShstAbstractService.h"
#include "../ShstUsbDisk.h"

class ShstUsbDiskService : public ShstAbstractService
{
    Q_OBJECT
public:
    explicit ShstUsbDiskService(QObject *parent = nullptr);
    ~ShstUsbDiskService() override;

    bool init() override;
    bool uninit() override;
    bool processTextMessage(QWebSocket* webSocket, QString message) override;
    bool processBinaryMessage(QWebSocket* webSocket, QByteArray message) override;

private:
    bool sendJsonResponse(QWebSocket* ws, const QJsonObject &response);
    bool getUsbDiskCount(const QString& action, QJsonObject &response) const;
};

#endif // SHSTUSBDISKSERVICE_H
