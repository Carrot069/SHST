#include "WzMcuQml.h"

QString WzMcuQml::getServerAddress() const
{
    return m_serverAddress;
}

void WzMcuQml::setServerAddress(const QString &serverAddress)
{
    m_serverAddress = serverAddress;
    if (nullptr != m_libusb)
        m_libusb->setServerAddress(serverAddress);
}

void WzMcuQml::execCmd(const QJsonObject &action)
{
    m_libusb->exec(QJsonDocument(action).toJson());
}

void WzMcuQml::execCmd(const QString &cmd)
{
    QJsonObject action;
    action["action"] = "exec";
    action["cmd"] = cmd;
    execCmd(action);
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

bool WzMcuQml::getConnected() const
{
    return m_connected;
}

void WzMcuQml::closeUV() {
    if ((m_latestLightType == "uv_penetrate_force" || m_latestLightType == "uv_penetrate")
            && m_latestLightOpened) {
        m_isDisableLightRsp = true;
        closeAllLight();
        WzUtils::sleep(300);
    }
}

WzMcuQml::WzMcuQml(QObject *parent) : QObject(parent) {
    qDebug() << "WzMcuQml created";

    m_virtualCmdReplyTimer = new QTimer(this);
    m_virtualCmdReplyTimer->setInterval(300);
    m_virtualCmdReplyTimer->setSingleShot(true);
    connect(m_virtualCmdReplyTimer, SIGNAL(timeout()), this, SLOT(virtualCmdReplyTimerTimeout()));
}

WzMcuQml::~WzMcuQml() {
    if (nullptr != m_virtualCmdReplyTimer) {
        delete m_virtualCmdReplyTimer;
        m_virtualCmdReplyTimer = nullptr;
    }
    qDebug() << "~WzMcuQml";
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

void WzMcuQml::rsp(const QString& rsp) {
    if (rsp == "libusb_init") {
        execCmd("enum_devices,12430,6100,12430,30024,12430,2100,22945,6100");
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

            execCmd(cmd);
        } else {
            // TODO
        }
    } else if (rsp.startsWith("open_device")) {
        // TODO 处理错误信息
        QByteArray jsonByteArray = rsp.right(rsp.length() - QString("open_device,").length()).toUtf8();
        QJsonDocument json = QJsonDocument::fromJson(jsonByteArray);
        m_openId = json["open_id"].toInt();
        qInfo() << "USB设备已打开, open_id:" << m_openId;
        m_connected = true;
        emit connectedChanged();
    } else if (rsp.startsWith("data_received")) {
        if (rsp.startsWith("data_received,5a,a5,0,5,3,67,"))
            emit lightSwitched("white_down", true);
        else if (rsp.startsWith("data_received,5a,a5,0,5,3,68,"))
            emit lightSwitched("white_up", true);
        else if (rsp.startsWith("data_received,5a,a5,0,5,3,6b,"))
            emit lightSwitched("uv_penetrate", true);
        else if (rsp.startsWith("data_received,5a,a5,0,5,3,75")) {
            if (m_isDisableLightRsp) {
                m_isDisableLightRsp = false;
                return;
            }
            emit lightSwitched(m_latestLightType, false);
            m_latestLightOpened = false;
        // door is opened
        } else if (rsp.startsWith("data_received,5a,a5,0,5,a,a8,7f,7b")) {
            if (m_latestLightType == "uv_penetrate") {
                closeAllLight();
            }
            emit doorOpened();
        } else if (rsp.startsWith("data_received,5a,a5,0,5,a,a8,6f,6b"))
            emit doorClosed();
        qDebug() << "收到数据: " << rsp;
    } else if (rsp.startsWith("server_disconnected")) {
        m_connected = false;
        emit connectedChanged();
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
    QJsonObject action;
    action["action"] = "switchLight";
    action["lightType"] = lightType;
    action["isOpen"] = isOpen;
    execCmd(action);
}

void WzMcuQml::closeAllLight() {
    if (m_openId == -1) return;
    QString cmd;
    cmd = "5a,a5,00,02,03,75,aa,bb";
    cmd = QString("send_data,%1,%2").arg(m_openId).arg(cmd);
    qDebug() << cmd;
    execCmd(cmd);
}

void WzMcuQml::init() {
    if (nullptr == m_libusb) {
        m_libusb = new WzLibusbNet();
        connect(m_libusb, &WzLibusbNet::rsp, this, &WzMcuQml::rsp);
        m_libusb->setServerAddress(m_serverAddress);
        m_libusb->start();
    }
    //m_libusb->exec("libusb_init");
}

void WzMcuQml::uninit() {
    //m_libusb->exec("libusb_uninit");
    if (nullptr != m_libusb) {
        disconnect(m_libusb, SIGNAL(rsp(const QString&)), this, SLOT(rsp(const QString&)));
        m_libusb->stop();
        delete m_libusb;
        m_libusb = nullptr;
    }
}

void WzMcuQml::focusStartFar() {
    if (m_openId == -1) return;
#ifdef GEL_CAPTURE
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,64,%2,%3,aa,bb").arg(m_openId).arg("00").arg("00");
#else
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,65,%2,%3,aa,bb").arg(m_openId).arg("00").arg("00");
#endif
    qDebug() << cmd;
    execCmd(cmd);
};

void WzMcuQml::focusStartNear() {
    if (m_openId == -1) return;
#ifdef GEL_CAPTURE
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,63,%2,%3,aa,bb").arg(m_openId).arg("00").arg("00");
#else
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,62,%2,%3,aa,bb").arg(m_openId).arg("00").arg("00");
#endif
    qDebug() << cmd;
    execCmd(cmd);
};

void WzMcuQml::focusStop() {
    if (m_openId == -1) return;
    QJsonObject action;
    action["action"] = "focusStop";
    execCmd(action);
}

void WzMcuQml::switchDoor()
{
    if (m_openId == -1) return;
    QJsonObject action;
    action["action"] = "switchDoor";
    execCmd(action);
};

void WzMcuQml::switchAperture(const int& apertureIndex) {
    if (m_openId == -1) return;
    m_aperture = apertureIndex;
    QJsonObject action;
    action["action"] = "switchAperture";
    action["apertureIndex"] = apertureIndex;
    execCmd(action);
};

void WzMcuQml::switchFocus(const int& focusIndex) {
    if (m_openId == -1) return;
    QStringList focusIndexs = {"77", "76", "75", "74", "73", "72", "71"};
    QString focusIndexStr = focusIndexs[focusIndex % 7];
    QString cmd = QString("send_data,%1,5a,a5,00,02,01,%2,aa,bb").arg(m_openId).arg(focusIndexStr);
    QJsonObject action;
    action["action"] = "switchFocus";
    action["focusIndex"] = focusIndex;
    execCmd(action);
};

void WzMcuQml::switchFilterWheel(const int& filterIndex) {
    if (m_openId == -1) return;
    m_filter = filterIndex;
    QString filterIndexs[] = {"6c", "6d", "6e", "6f", "70", "71", "72", "73"};
    QString filterIndexStr = filterIndexs[filterIndex % 8];
    QString cmd = QString("send_data,%1,5a,a5,00,02,02,%3,aa,bb").arg(m_openId).arg(filterIndexStr);
    QJsonObject action;
    action["action"] = "switchFilterWheel";
    action["filterIndex"] = filterIndex;
    execCmd(action);
};
