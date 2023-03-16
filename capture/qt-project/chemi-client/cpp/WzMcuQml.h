#ifndef WZMCUQML_H
#define WZMCUQML_H

#include <QObject>
#include <QtGlobal>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>

#include "WzLibusbNet.h"
#include "WzUtils.h"

class WzMcuQml : public QObject
{
    Q_OBJECT
private:
    WzLibusbNet *m_libusb = nullptr;
    int m_openId = -1;
    QString m_latestLightType;
    bool m_latestLightOpened = false;
    bool m_isDisableLightRsp = false;
    int m_aperture = 0;
    int m_filter = 0;
    QTimer* m_virtualCmdReplyTimer = nullptr;
    QString m_serverAddress;
    bool m_connected = false;

    QString getLatestLightType();
    bool getLatestLightOpened();
    int getAperture();
    int getFilter();
    bool getConnected() const;

    void closeUV();

    QString getServerAddress() const;
    void setServerAddress(const QString &serverAddress);
    void execCmd(const QJsonObject &action);
    void execCmd(const QString &cmd);

public:
    explicit WzMcuQml(QObject *parent = nullptr);
    ~WzMcuQml() override;

    Q_INVOKABLE void focusStartFar();
    Q_INVOKABLE void focusStartNear();
    Q_INVOKABLE void focusStop();
    Q_INVOKABLE void switchDoor();
    Q_INVOKABLE void switchAperture(const int& apertureIndex);
    Q_INVOKABLE void switchFocus(const int& focusIndex);
    Q_INVOKABLE void switchFilterWheel(const int& filterIndex);
    Q_INVOKABLE void switchLight(const QString& lightType, const bool& isOpen);
    Q_INVOKABLE void closeAllLight();
    Q_INVOKABLE void init();
    Q_INVOKABLE void uninit();

    Q_PROPERTY(QString latestLightType READ getLatestLightType)
    Q_PROPERTY(bool latestLightOpened READ getLatestLightOpened)
    Q_PROPERTY(int aperture READ getAperture)
    Q_PROPERTY(int filter READ getFilter)
    Q_PROPERTY(QString serverAddress READ getServerAddress WRITE setServerAddress)
    Q_PROPERTY(bool connected READ getConnected NOTIFY connectedChanged)

signals:
    void lightSwitched(const QString& lightType, const bool& isOpened);
    void doorOpened();
    void doorClosed();
    void connectedChanged();

private slots:
    void virtualCmdReplyTimerTimeout();

public slots:
    void rsp(const QString& rsp);
};

#endif // WZMCUQML_H
