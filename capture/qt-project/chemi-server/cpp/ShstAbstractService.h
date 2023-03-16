#ifndef SHSTABSTRACTSERVICE_H
#define SHSTABSTRACTSERVICE_H

#include <QObject>
#include <QWebSocket>

class ShstAbstractService : public QObject
{
    Q_OBJECT
public:
    explicit ShstAbstractService(QObject *parent = nullptr);

    virtual bool init() = 0;
    virtual bool uninit() = 0;
    virtual bool processTextMessage(QWebSocket* webSocket, QString message) = 0;
    virtual bool processBinaryMessage(QWebSocket* webSocket, QByteArray message) = 0;
};

#endif // SHSTABSTRACTSERVICE_H
