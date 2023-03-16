#ifndef WZMCUQML_H
#define WZMCUQML_H

#include <QObject>
#include <QtGlobal>
#include <QTimer>
#include <QJsonArray>
#include "WzLibusbThread.h"
#include "WzUtils.h"
#include "WzDatabaseService.h"
#include "WzIniSetting.h"

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
    int m_aperture = 0;
    int m_filter = 0;
    QTimer* m_focusStopTimer = nullptr;
    int m_focusStopCount = 0;
    QTimer* m_virtualCmdReplyTimer = nullptr;
    // 以下代码是为了暂时解决滤镜轮乱转的问题, 解决思路是等最后一次单片机指令之后的
    // 几秒内再发送一次滤镜轮转动指令
    // 最后一次发送指令的时间
    QDateTime m_latestCmdTime;
    // 最后一次发送的滤镜轮指令
    QString m_latestFilterCmd;
    bool m_repeatSetFilter = false;
    // 为true时表示允许自动调整滤镜轮, false表示不允许, 比如正在聚焦时就不能自动发送调整滤镜轮的指令
    bool m_repeatSetFilterInternal = true;
    QTimer* m_repeatSetFilterTimer = nullptr;
    // end //

    QString getLatestLightType();
    bool getLatestLightOpened();
    int getAperture();
    int getFilter();
    bool getRepeatSetFilter() const;
    void setRepeatSetFilter(bool repeatSetFilter);
    bool getConnected() const;

    void closeUV();
    QString getVidPid() const;

    void parsePowerOnTick(const QString &rsp);

    // 2021-11-10
    bool m_cachedIniCmd = true;
    QString m_cmdUvReflex1 = "5a,a5,00,04,03,76,07,08,aa,bb";
    QString m_cmdUvReflex2 = "5a,a5,00,04,03,74,07,08,aa,bb";
    // 2021-11-10
public:
    explicit WzMcuQml(QObject *parent = nullptr);
    ~WzMcuQml() override;

    Q_INVOKABLE void focusFar(const int step);
    Q_INVOKABLE void focusNear(const int step);
    Q_INVOKABLE void focusStartFar();
    Q_INVOKABLE void focusStartNear();
    Q_INVOKABLE void focusStop();
    Q_INVOKABLE void switchAperture(const int& apertureIndex);
    Q_INVOKABLE void switchFocus(const int& focusIndex);
    Q_INVOKABLE void switchFilterWheel(const int& filterIndex);
    Q_INVOKABLE void switchLight(const QString& lightType, const bool& isOpen);
    Q_INVOKABLE void closeAllLight();
    Q_INVOKABLE void init();
    Q_INVOKABLE void uninit();
    Q_INVOKABLE void getPowerOnTick();
    Q_INVOKABLE int getOpenId() const;
    Q_INVOKABLE void exec(const QString &cmd);

    Q_INVOKABLE void openDevice(const int& bus, const int &address);

    Q_PROPERTY(bool connected READ getConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString latestLightType READ getLatestLightType NOTIFY latestLightTypeChanged)
    Q_PROPERTY(bool latestLightOpened READ getLatestLightOpened NOTIFY latestLightOpenedChanged)
    Q_PROPERTY(int aperture READ getAperture NOTIFY apertureChanged)
    Q_PROPERTY(int filter READ getFilter NOTIFY filterChanged)
    Q_PROPERTY(bool repeatSetFilter READ getRepeatSetFilter WRITE setRepeatSetFilter NOTIFY repeatSetFilterChanged)
    Q_PROPERTY(int availableCount MEMBER m_availableCount NOTIFY availableCountChanged)
    Q_PROPERTY(QJsonArray devices MEMBER m_devices NOTIFY devicesChanged)

signals:
    void lightSwitched(const QString& lightType, const bool& isOpened);
    void doorOpened();
    void doorClosed();    
    void opened(const QString &sn);
    void error(const QString &errorMsg);
    void response(const QString& response);
    // notify
    void availableCountChanged();
    void devicesChanged();
    void connectedChanged();

    void latestLightTypeChanged();
    void latestLightOpenedChanged();
    void apertureChanged();
    void filterChanged();
    void repeatSetFilterChanged();

    void powerOnTick(const int tick);

private slots:
    void focusStopTimerTimeout();
    void virtualCmdReplyTimerTimeout();
    void setFilterTimerTimeout();
    void cmdExecBefore(const QString &cmd);

public slots:
    void rsp(const QString& rsp);
};

#endif // WZMCUQML_H
