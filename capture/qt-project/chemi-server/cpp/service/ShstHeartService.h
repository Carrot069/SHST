#ifndef SHSTHEARTSERVICE_H
#define SHSTHEARTSERVICE_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMapIterator>
#include <QDateTime>

#include "../ShstAbstractService.h"

class ShstHeartService : public ShstAbstractService
{
public:
    explicit ShstHeartService(QObject *parent = nullptr);
    ~ShstHeartService() override;

    bool init() override;
    bool uninit() override;
    bool processTextMessage(QWebSocket* webSocket, QString message) override;
    bool processBinaryMessage(QWebSocket* webSocket, QByteArray message) override;

private:
    const int TIMEOUT_MSEC = 5000;
    QMap<QString, QDateTime> m_alives;
    bool heart(QWebSocket* webSocket, QString message);
    bool getAliveCount(QWebSocket* webSocket, QString message);

    // tool functions
    bool isHeartMsg(const QString& message);
    QString getHeartKey(const QWebSocket* webSocket);
    void updateHeart(QString& key);
    bool isHeartTimeout(const QDateTime& dt);
};

#endif // SHSTHEARTSERVICE_H
