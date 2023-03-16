#include "WzMcuQml.h"

bool WzMcuQml::isDoorOpened() const
{
    return m_isDoorOpened;
}

void WzMcuQml::switchDoor()
{
    if (m_latestDoorCommandTime.addSecs(3) > QDateTime::currentDateTime()) {
        qInfo() << "开关门动作间隔小于3秒, 停止执行";
        return;
    }
    m_latestDoorCommandTime = QDateTime::currentDateTime();
    m_doorControlCommands.clear();
    if (m_isDoorOpened) {
        m_doorControlCommands << CMD_CLOSE_DOOR;
    } else {
        // 如果是关门的状态, 那这次需要开门, 如果是紫外开着就需要关闭它
        if (m_latestLightOpened && m_latestLightType == "uv_penetrate") {
            m_doorControlCommands << CMD_CLOSE_ALL_LIGHT;
        }
        m_doorControlCommands << CMD_OPEN_DOOR;
    }
    m_isDoorOpened = !m_isDoorOpened;
    execCmd(CMD_STOP_DOOR);
    m_openCloseDoorTimer->start();
}

QString WzMcuQml::getLatestLightType() {
    return m_latestLightType;
}

bool WzMcuQml::getLatestLightOpened() {
    return m_latestLightOpened;
}

int WzMcuQml::getAperture()
{
    return m_aperture;
}

int WzMcuQml::getFilter()
{
    return m_filter;
}

void WzMcuQml::closeUV() {
    if ((m_latestLightType == "uv_penetrate_force" || m_latestLightType == "uv_penetrate")
            && m_latestLightOpened) {
        m_isDisableLightRsp = true;
        closeAllLight();
        WzUtils::sleep(300);
    }
}

bool WzMcuQml::execCmd(QString cmd)
{
    if (m_openId == -1)
        return false;
    cmd = QString("send_data,%1,%2").
                arg(m_openId).arg(cmd);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);

    return true;
}

void WzMcuQml::readPorts()
{
    auto readPort = [](const QProcessEnvironment &pe, const QString &key, int &port) {
        qInfo() << "read port, " << key;
        if (pe.contains(key)) {
            qInfo() << "\tcontains true";
            QString portStr = pe.value(key);
            int p;
            bool ok;
            p = portStr.toInt(&ok);
            if (ok) {
                port = p;
                qInfo() << "\tport is" << port;
            }
        } else {
            qInfo() << "\t" << "contains false";
        }
    };
    QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();
    readPort(pe, "SHSTServerMcuPort", m_webSocketServerPort);
    WzUdpBroadcastSender::McuPort = m_webSocketServerPort;
}

WzMcuQml::WzMcuQml(QObject *parent) : QObject(parent) {
    qInfo() << "WzMcuQml created";

    readPorts();

    m_focusStopTimer = new QTimer(this);
    m_focusStopTimer->setInterval(100);
    m_focusStopTimer->setSingleShot(false);
    connect(m_focusStopTimer, SIGNAL(timeout()), this, SLOT(focusStopTimerTimeout()));

    m_virtualCmdReplyTimer = new QTimer(this);
    m_virtualCmdReplyTimer->setInterval(300);
    m_virtualCmdReplyTimer->setSingleShot(true);
    connect(m_virtualCmdReplyTimer, SIGNAL(timeout()), this, SLOT(virtualCmdReplyTimerTimeout()));

    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("/mcu/control"), QWebSocketServer::NonSecureMode);
    if (m_pWebSocketServer->listen(QHostAddress::Any, m_webSocketServerPort)) {
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &WzMcuQml::onWebSocketNewConnection);
    } else {
        qWarning() << "Mcu, listen failure";
    }

    m_openCloseDoorTimer = new QTimer(this);
    connect(m_openCloseDoorTimer, &QTimer::timeout, this, &WzMcuQml::openCloseDoorTimerTimeout);
    m_openCloseDoorTimer->setInterval(300);
    m_openCloseDoorTimer->setSingleShot(false);
}

WzMcuQml::~WzMcuQml() {
    if (nullptr != m_pWebSocketServer) {
        delete m_pWebSocketServer;
        m_pWebSocketServer = nullptr;
    }
    if (nullptr != m_virtualCmdReplyTimer) {
        delete m_virtualCmdReplyTimer;
        m_virtualCmdReplyTimer = nullptr;
    }
    if (m_focusStopTimer != nullptr) {
        delete m_focusStopTimer;
        m_focusStopTimer = nullptr;
    }
    qInfo() << "~WzMcuQml";
}

void WzMcuQml::focusFar(const int step, const bool isGel)
{
    if (m_openId == -1) return;
    uint8_t stepHigh = (step & 0xFF00) >> 8;
    uint8_t stepLow = (step & 0xFF);
    QString cmd;
    if (isGel)
        cmd = QString("send_data,%1,5a,a5,00,04,01,64,%2,%3,aa,bb").
                arg(m_openId).
                arg(QString::asprintf("%.2X", stepHigh),
                    QString::asprintf("%.2X", stepLow));
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

void WzMcuQml::focusNear(const int step, const bool isGel)
{
    if (m_openId == -1) return;
    uint8_t stepHigh = (step & 0xFF00) >> 8;
    uint8_t stepLow = (step & 0xFF);
    QString cmd;
    if (isGel)
        cmd = QString("send_data,%1,5a,a5,00,04,01,63,%2,%3,aa,bb").
                arg(m_openId).
                arg(QString::asprintf("%.2X", stepHigh),
                    QString::asprintf("%.2X", stepLow));
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
}

void WzMcuQml::focusStopTimerTimeout() {
    if (m_openId == -1) return;
    QString cmd = QString("send_data,%1,5a,a5,00,02,01,67,aa,bb").arg(m_openId);
    qDebug() << "repeat stop focus, " << cmd;
    m_libusbThread->exec(cmd);
    m_focusStopCount++;
    if (m_focusStopCount == 3)
        m_focusStopTimer->stop();
}

void WzMcuQml::virtualCmdReplyTimerTimeout()
{
    // 如果最后打开的是荧光则模拟单片机返回指令, 目前的板子还不能实际返回, 2020-5-21 by wangzhe
    if (m_latestLightOpened) {
        if (m_latestLightType == "red")
            emit lightSwitched("red", true);
        else if (m_latestLightType == "green")
            emit lightSwitched("green", true);
        else if (m_latestLightType == "blue")
            emit lightSwitched("blue", true);
    }
}

void WzMcuQml::openCloseDoorTimerTimeout()
{
    qInfo() << "openCloseDoorTimerTimeout";
    if (m_doorControlCommands.count() == 0) {
        m_openCloseDoorTimer->stop();
        return;
    }
    execCmd(m_doorControlCommands.first());
    m_doorControlCommands.removeFirst();
}

void WzMcuQml::onWebSocketNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    qDebug() << "onWebSocketNewConnection" << pSocket;

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WzMcuQml::textMessageReceived);
    connect(pSocket, &QWebSocket::disconnected, this, &WzMcuQml::onWebSocketDisconnected);

    if ("" != m_openDeviceRsp)
        pSocket->sendTextMessage(m_openDeviceRsp);

    m_clients << pSocket;
}

void WzMcuQml::onWebSocketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    qDebug() << "webSocketDisconnected:" << pClient->peerAddress();
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

void WzMcuQml::textMessageReceived(const QString &message)
{
    qDebug() << "McuQml::textMessageReceived" << message;
    if (nullptr == m_libusbThread)
        return;

    QJsonDocument action = QJsonDocument::fromJson(message.toUtf8());
    QString act = action["action"].toString();
    if (act == "exec") {
        m_libusbThread->exec(action["cmd"].toString());
    } else if (act == "switchLight") {
        switchLight(action["lightType"].toString(), action["isOpen"].toBool());
    } else if (act == "focusStop") {
        focusStop();
    } else if (act == "switchAperture") {
        switchAperture(action["apertureIndex"].toInt());
    } else if (act == "switchFocus") {
        switchFocus(action["focusIndex"].toInt());
    } else if (act == "switchFilterWheel") {
        switchFilterWheel(action["filterIndex"].toInt());
    } else if (act == "switchDoor") {
        switchDoor();
    }
}

void WzMcuQml::rsp(const QString& rsp) {
    for (int i = 0; i < m_clients.count(); i++)
        m_clients.at(i)->sendTextMessage(rsp);
    if (rsp == "libusb_init") {
        m_libusbThread->exec("enum_devices,12430,6100,12430,30024,12430,2100,22945,6100,22945,2100");
    } else if (rsp.startsWith("enum_devices")) {
        QByteArray jsonByteArray = rsp.right(rsp.length() - 13).toUtf8();
        QJsonDocument json = QJsonDocument::fromJson(jsonByteArray);

        // 只有一个设备, 直接打开
        qDebug() << json["count"];
        if (json["count"].toDouble() == 1.0) {
            double bus = json["devices"][0]["bus"].toDouble();
            double address = json["devices"][0]["address"].toDouble();
            QString cmd = QString("open_device,%1,%2").arg(bus).arg(address);

            qInfo() << "只有一个USB设备, 直接打开";

            m_libusbThread->exec(cmd);
        } else {
            // TODO
        }
    } else if (rsp.startsWith("open_device")) {
        // TODO 处理错误信息
        QByteArray jsonByteArray = rsp.right(rsp.length() - QString("open_device,").length()).toUtf8();
        QJsonDocument json = QJsonDocument::fromJson(jsonByteArray);
        m_openId = json["open_id"].toInt();
        qInfo() << "USB设备已打开, open_id:" << m_openId;
        m_openDeviceRsp = rsp;
    } else if (rsp.startsWith("data_received")) {
        qDebug() << "收到数据: " << rsp;

        if (rsp.startsWith("data_received,5a,a5,0,5,3,67,")) {
            m_latestLightType = "white_down";
            emit lightSwitched("white_down", true);
        } else if (rsp.startsWith("data_received,5a,a5,0,5,3,68,")) {
            m_latestLightType = "white_up";
            emit lightSwitched("white_up", true);
        } else if (rsp.startsWith("data_received,5a,a5,0,5,3,6b,")) {
            m_latestLightType = "uv_penetrate";
            emit lightSwitched("uv_penetrate", true);
        } else if (rsp.startsWith("data_received,5a,a5,0,5,3,75")) {
            if (m_isDisableLightRsp) {
                m_isDisableLightRsp = false;
                return;
            }
            emit lightSwitched(m_latestLightType, false);
            m_latestLightOpened = false;
        // 感应开关的物体靠近
        } else if (rsp.startsWith("data_received,5a,a5,0,5,a,a8,7f,7b")) {
            //if (m_latestLightType == "uv_penetrate") {
            //    closeAllLight();
            //}
            emit doorSwitchEnter();
        } else if (rsp.startsWith("data_received,5a,a5,0,5,a,a8,6f,6b"))
            emit doorSwitchLeave();

    }
}

void WzMcuQml::switchLight(const QString& lightType, const bool& isOpen) {
    if (m_openId == -1) return;

    QString cmd;
    if (isOpen) {
        if (lightType == "white_up") {
            cmd = "5a,a5,00,04,03,68,55,55,aa,bb";
        } else if (lightType == "white_down") {
            cmd = "5a,a5,00,04,03,67,55,55,aa,bb";
        } else if (lightType == "uv_penetrate") {
            cmd = "5a,a5,00,04,03,6b,ff,ff,aa,bb";
        } else if (lightType == "uv_penetrate_force") {
            cmd = "5a,a5,00,04,03,8b,ff,ff,aa,bb";
        } else if (lightType == "blue_penetrate") {
            cmd = "5a,a5,00,04,03,69,ff,ff,aa,bb";
        } else if (lightType == "red") {
            cmd = "5a,a5,00,04,03,7a,07,08,aa,bb";
            m_virtualCmdReplyTimer->start();
        } else if (lightType == "green") {
            cmd = "5a,a5,00,04,03,79,07,08,aa,bb";
            m_virtualCmdReplyTimer->start();
        } else if (lightType == "blue") {
            cmd = "5a,a5,00,04,03,78,07,08,aa,bb";
            m_virtualCmdReplyTimer->start();
        }
        m_latestLightOpened = true;
    } else {
        cmd = "5a,a5,00,02,03,75,aa,bb";
        m_latestLightOpened = false;
    }
    m_latestLightType = lightType;
    cmd = QString("send_data,%1,%2").arg(m_openId).arg(cmd);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
}

void WzMcuQml::closeAllLight() {
    if (m_openId == -1) return;
    QString cmd;
    cmd = QString("send_data,%1,%2").arg(m_openId).arg(CMD_CLOSE_ALL_LIGHT);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
}

void WzMcuQml::init() {
    if (nullptr == m_libusbThread) {
        m_libusbThread = new LibusbThread();
        connect(m_libusbThread, SIGNAL(rsp(const QString&)), this, SLOT(rsp(const QString&)));
        m_libusbThread->start();
    }
    m_libusbThread->exec("libusb_init");
}

void WzMcuQml::uninit() {
    m_libusbThread->exec("libusb_uninit");
    if (nullptr != m_libusbThread) {
        disconnect(m_libusbThread, SIGNAL(rsp(const QString&)), this, SLOT(rsp(const QString&)));
        m_libusbThread->stop();
        m_libusbThread->wait();
        // 不需要删除, 该线程已连接了 signal finished 与 QObject::deleteLater
        m_libusbThread = nullptr;
    }
}

void WzMcuQml::focusStartFar() {
    if (m_openId == -1) return;
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,65,%2,%3,aa,bb").arg(m_openId).arg("00").arg("00");
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

void WzMcuQml::focusStartNear() {
    if (m_openId == -1) return;
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,62,%2,%3,aa,bb").arg(m_openId).arg("00").arg("00");
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

void WzMcuQml::focusStop() {
    if (m_openId == -1) return;
    QString cmd = QString("send_data,%1,5a,a5,00,02,01,67,aa,bb").arg(m_openId);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
    m_focusStopCount = 0;
    m_focusStopTimer->start();
};

void WzMcuQml::switchAperture(const int& apertureIndex) {
    if (m_openId == -1) return;
    m_aperture = apertureIndex;
    QStringList apertureIndexs = {"57", "56", "55", "54", "53", "52", "51"};
    QString apertureIndexStr = apertureIndexs[apertureIndex % 7];
    QString cmd = QString("send_data,%1,5a,a5,00,02,01,%2,aa,bb").arg(m_openId).arg(apertureIndexStr);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

void WzMcuQml::switchFocus(const int& focusIndex) {
    if (m_openId == -1) return;
    QStringList focusIndexs = {"77", "76", "75", "74", "73", "72", "71"};
    QString focusIndexStr = focusIndexs[focusIndex % 7];
    QString cmd = QString("send_data,%1,5a,a5,00,02,01,%2,aa,bb").arg(m_openId).arg(focusIndexStr);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

void WzMcuQml::switchFilterWheel(const int& filterIndex) {
    if (m_openId == -1) return;
    m_filter = filterIndex;
    QString filterIndexs[] = {"6c", "6d", "6e", "6f", "70", "71", "72", "73"};
    QString filterIndexStr = filterIndexs[filterIndex % 8];
    QString cmd = QString("send_data,%1,5a,a5,00,02,02,%3,aa,bb").arg(m_openId).arg(filterIndexStr);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};
