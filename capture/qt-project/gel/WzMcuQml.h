#ifndef WZMCUQML_H
#define WZMCUQML_H

#include <QObject>
#include <QtGlobal>
#include <QTimer>
#include <QJsonArray>
#include "WzLibusbThread.h"
#include "WzDatabaseService.h"
#include "WzUtils.h"

class WzMcuQml : public QObject
{
    Q_OBJECT
private:
    bool m_connected = false;
    int m_availableCount = 0;
    QJsonArray m_devices;
    LibusbThread *m_libusbThread = nullptr;
    int m_openId = -1;
    QString m_latestLightType;
    bool m_latestLightOpened = false;
    bool m_isDisableLightRsp = false;
    QTimer* m_focusStopTimer = nullptr;
    int m_focusStopCount = 0;

    QString getLatestLightType();
    bool getLatestLightOpened();
    bool getConnected() const;

    void closeUV();
    QString getVidPid() const;

public:
    explicit WzMcuQml(QObject *parent = nullptr);
    ~WzMcuQml() override;

    Q_INVOKABLE void focusFar(const int step);
    Q_INVOKABLE void focusNear(const int step);
    Q_INVOKABLE void focusStartFar();
    Q_INVOKABLE void focusStartNear();
    // 化学发光镜头的指令
    Q_INVOKABLE void focusStartFar2(const qreal clickPos);
    Q_INVOKABLE void focusStartNear2(const qreal clickPos);
    Q_INVOKABLE void focusStop();
    Q_INVOKABLE void switchAperture(const int& filterIndex);
    Q_INVOKABLE void switchFocus(const int& focusIndex);
    Q_INVOKABLE void switchFilterWheel(const int& filterIndex);
    Q_INVOKABLE void switchLight(const QString& lightType, const bool& isOpen);
    Q_INVOKABLE void closeAllLight();
    Q_INVOKABLE void init();
    Q_INVOKABLE void uninit();
    //Q_INVOKABLE void

    Q_INVOKABLE void openDevice(const int& bus, const int &address);

    Q_PROPERTY(bool connected READ getConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString latestLightType READ getLatestLightType NOTIFY latestLightTypeChanged)
    Q_PROPERTY(bool latestLightOpened READ getLatestLightOpened NOTIFY latestLightOpenedChanged)
    Q_PROPERTY(int availableCount MEMBER m_availableCount NOTIFY availableCountChanged)
    Q_PROPERTY(QJsonArray devices MEMBER m_devices NOTIFY devicesChanged)

signals:
    void lightSwitched(const QString& lightType, const bool& isOpened);
    void doorOpened();
    void doorClosed();    
    void opened(const QString &sn);
    void error(const QString &errorMsg);
    // notify
    void availableCountChanged();
    void devicesChanged();
    void connectedChanged();
    void latestLightTypeChanged();
    void latestLightOpenedChanged();

private slots:
    void focusStopTimerTimeout();

public slots:
    void rsp(const QString& rsp);

// 低速聚焦相关, 用定时器重复发送转动特定步数的指令
private:
    bool m_lowSpeedFocus = false;
    QString m_focusCmd;
    QTimer m_focusTimer;
private slots:
    void focusTimerTimeout();
};

#endif // WZMCUQML_H
