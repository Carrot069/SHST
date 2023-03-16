#ifndef WZMCUQML_H
#define WZMCUQML_H

#include <QObject>
#include <QtGlobal>
#include <QTimer>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QProcessEnvironment>

#include "WzLibusbThread.h"
#include "WzUtils.h"
#include "WzUdpBroadcastSender.h"

const QString CMD_CLOSE_ALL_LIGHT = "5a,a5,00,02,03,75,aa,bb";
const QString CMD_OPEN_DOOR = "5a,a5,00,04,07,63,0,0,aa,bb";
const QString CMD_CLOSE_DOOR = "5a,a5,00,04,07,64,0,0,aa,bb";
const QString CMD_STOP_DOOR = "5a,a5,00,04,07,67,0,0,aa,bb";

class WzMcuQml : public QObject
{
    Q_OBJECT
private:
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

    // 控制电动抽屉的
    bool m_isDoorOpened = false;
    // 目前(2021-4-28)单片机不能在300毫秒内响应两次指令, 所以停止电机和开始转动电机两个指令之间需要定时器间隔发送
    QTimer* m_openCloseDoorTimer = nullptr;
    QStringList m_doorControlCommands;    
    QDateTime m_latestDoorCommandTime;
    bool isDoorOpened() const;

    QString getLatestLightType();
    bool getLatestLightOpened();
    int getAperture();
    int getFilter();

    void closeUV();

    bool execCmd(QString cmd);

    // websocket server
    QWebSocketServer *m_pWebSocketServer = nullptr;
    QList<QWebSocket*> m_clients;
    int m_webSocketServerPort = 60053;
    // 打开USB设备后的响应指令, 备份下来之后在新WebSocket客户端建立连接后将其发送过去表示服务端已经打开了设备
    QString m_openDeviceRsp;
    // websocket server

    void readPorts();
public:
    explicit WzMcuQml(QObject *parent = nullptr);
    ~WzMcuQml() override;

    Q_INVOKABLE void focusFar(const int step, const bool isGel = false);
    Q_INVOKABLE void focusNear(const int step, const bool isGel = false);
    Q_INVOKABLE void focusStartFar();
    Q_INVOKABLE void focusStartNear();
    Q_INVOKABLE void focusStop();
    Q_INVOKABLE void switchAperture(const int& apertureIndex);
    Q_INVOKABLE void switchFocus(const int& focusIndex);
    Q_INVOKABLE void switchFilterWheel(const int& filterIndex);
    Q_INVOKABLE void switchLight(const QString& lightType, const bool& isOpen);
    Q_INVOKABLE void closeAllLight();
    Q_INVOKABLE void switchDoor();
    Q_INVOKABLE void init();
    Q_INVOKABLE void uninit();    

    Q_PROPERTY(QString latestLightType READ getLatestLightType)
    Q_PROPERTY(bool latestLightOpened READ getLatestLightOpened)
    Q_PROPERTY(int aperture READ getAperture)
    Q_PROPERTY(int filter READ getFilter)
    Q_PROPERTY(bool isDoorOpened READ isDoorOpened)

signals:
    void lightSwitched(const QString& lightType, const bool& isOpened);
    void doorSwitchEnter();
    void doorSwitchLeave();

private slots:
    void focusStopTimerTimeout();
    void virtualCmdReplyTimerTimeout();
    void openCloseDoorTimerTimeout();

    void onWebSocketNewConnection();
    void onWebSocketDisconnected();
    void textMessageReceived(const QString &message);

public slots:
    void rsp(const QString& rsp);
};

#endif // WZMCUQML_H
