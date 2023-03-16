#include "WzMcuQml.h"

bool WzMcuQml::getRepeatSetFilter() const
{
    return m_repeatSetFilter;
}

void WzMcuQml::setRepeatSetFilter(bool repeatSetFilter)
{
    m_repeatSetFilter = repeatSetFilter;
}

bool WzMcuQml::getConnected() const
{
    return m_connected;
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

QString WzMcuQml::getVidPid() const
{
    WzDatabaseService db;
    QString vidPid = db.readStrOption("vid_pid", "59A1,17D4");
    // 转换成十进制
    auto sl = vidPid.split(",");
    QStringList sl2;
    foreach(QString s, sl) {
        bool ok;
        sl2.append(QString::number(s.toInt(&ok, 16)));
    }
    qInfo() << "McuQml::getVidPid" << sl2.join(",");
    return sl2.join(",");
}

void WzMcuQml::parsePowerOnTick(const QString &rsp)
{
    // data_received,5a,a5,0,6,a,a2,xx,xx,xx,xx,aa,bb
    QStringList sl = rsp.split(",");
    if (sl.count() == 13) {
        int tick = (sl[7].toInt(nullptr, 16) & 0xFF) << 24 |
                   (sl[8].toInt(nullptr, 16) & 0xFF) << 16 |
                   (sl[9].toInt(nullptr, 16) & 0xFF) << 8 |
                   (sl[10].toInt(nullptr, 16) & 0xFF);
        qDebug() << "PowerOnTick:" << tick;
        emit powerOnTick(tick / 2);
    }
}

WzMcuQml::WzMcuQml(QObject *parent) : QObject(parent) {
    qDebug() << "WzMcuQml created";
    m_focusStopTimer = new QTimer(this);
    m_focusStopTimer->setInterval(100);
    m_focusStopTimer->setSingleShot(false);
    connect(m_focusStopTimer, SIGNAL(timeout()), this, SLOT(focusStopTimerTimeout()));

    m_virtualCmdReplyTimer = new QTimer(this);
    m_virtualCmdReplyTimer->setInterval(300);
    m_virtualCmdReplyTimer->setSingleShot(true);
    connect(m_virtualCmdReplyTimer, SIGNAL(timeout()), this, SLOT(virtualCmdReplyTimerTimeout()));

    m_latestCmdTime = QDateTime::currentDateTime();
    m_repeatSetFilterTimer = new QTimer(this);
    m_repeatSetFilterTimer->setInterval(500);
    m_repeatSetFilterTimer->setSingleShot(false);
    connect(m_repeatSetFilterTimer, &QTimer::timeout, this, &WzMcuQml::setFilterTimerTimeout);
    m_repeatSetFilterTimer->start();

    WzIniSetting iniSetting;
    m_cachedIniCmd = iniSetting.readBool("Cmd", "CachedIniCmd", true);
    if (m_cachedIniCmd) {
       m_cmdUvReflex1 = iniSetting.readStr("Cmd", "UvReflex1", m_cmdUvReflex1);
       m_cmdUvReflex2 = iniSetting.readStr("Cmd", "UvReflex2", m_cmdUvReflex2);
    }
}

WzMcuQml::~WzMcuQml() {
    if (nullptr != m_repeatSetFilterTimer) {
        delete m_repeatSetFilterTimer;
        m_repeatSetFilterTimer = nullptr;
    }

    if (nullptr != m_virtualCmdReplyTimer) {
        delete m_virtualCmdReplyTimer;
        m_virtualCmdReplyTimer = nullptr;
    }
    if (m_focusStopTimer != nullptr) {
        delete m_focusStopTimer;
        m_focusStopTimer = nullptr;
    }
    qDebug() << "~WzMcuQml";
}

void WzMcuQml::focusFar(const int step)
{
    if (m_openId == -1) return;
    uint8_t stepHigh = (step & 0xFF00) >> 8;
    uint8_t stepLow = (step & 0xFF);
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,65,%2,%3,aa,bb").
            arg(m_openId).
            arg(QString::asprintf("%.2X", stepHigh),
            QString::asprintf("%.2X", stepLow));
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

void WzMcuQml::focusNear(const int step)
{
    if (m_openId == -1) return;
    uint8_t stepHigh = (step & 0xFF00) >> 8;
    uint8_t stepLow = (step & 0xFF);
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,62,%2,%3,aa,bb").
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
    if (m_focusStopCount == 3) {
        m_focusStopTimer->stop();
        m_latestCmdTime = QDateTime::currentDateTime();
        m_repeatSetFilterInternal = true;
    }
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
        else if (m_latestLightType == "uv_reflex1")
            emit lightSwitched("uv_reflex1", true);
        else if (m_latestLightType == "uv_reflex2")
            emit lightSwitched("uv_reflex2", true);
    }
}

void WzMcuQml::setFilterTimerTimeout()
{
    if (!m_repeatSetFilter)
        return;
    if (!m_repeatSetFilterInternal)
        return;
    if ("" == m_latestFilterCmd)
        return;
    if (m_latestCmdTime.msecsTo(QDateTime::currentDateTime()) >= 3000) {
        qInfo() << "auto set filter";
        m_latestCmdTime = QDateTime::currentDateTime();
        m_libusbThread->exec(m_latestFilterCmd);
    }
}

void WzMcuQml::cmdExecBefore(const QString &cmd)
{
    Q_UNUSED(cmd);
    m_latestCmdTime = QDateTime::currentDateTime();
}

void WzMcuQml::rsp(const QString& rsp) {
    emit response(rsp);
    if (rsp == "libusb_init") {
        m_libusbThread->exec("enum_devices," + getVidPid());
    } else if (rsp.startsWith("enum_devices")) {
        QByteArray jsonByteArray = rsp.right(rsp.length() - 13).toUtf8();
        QJsonDocument json = QJsonDocument::fromJson(jsonByteArray);

        // 统计可用设备数量, 有sn的说明设备被成功打开, 否则就是打开失败没获取到
        int availableCount = 0;
        QJsonArray devices = json["devices"].toArray();
        qDebug() << "usb devices:" << json["count"];
        for (int i = 0; i < json["count"].toInt(); i++) {
            QString sn = devices.at(i)["sn"].toString();
            if (sn != "")
                availableCount++;
        }
        if (availableCount != m_availableCount) {
            m_availableCount = availableCount;
            emit availableCountChanged();
        }
        m_devices = devices;
        emit devicesChanged();
    } else if (rsp.startsWith("open_device")) {
        QByteArray jsonByteArray = rsp.right(rsp.length() - QString("open_device,").length()).toUtf8();
        QJsonDocument json = QJsonDocument::fromJson(jsonByteArray);
        if (json["error_number"].isUndefined()) {
            m_openId = json["open_id"].toInt();
            emit opened(json["sn"].toString());
            m_connected = true;
            emit connectedChanged();
            qInfo() << "USB设备已打开, open_id:" << m_openId << ", sn:" << json["sn"].toString();
        } else {
            emit error(json["error_msg"].toString());
        }
    } else if (rsp.startsWith("data_received")) {
        if (rsp.startsWith("data_received,5a,a5,0,5,3,67,"))
            if (m_latestLightType == "blue_penetrate" ||
                m_latestLightType == "blue_penetrate_force")
                emit lightSwitched(m_latestLightType, true);
            else
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
            emit latestLightOpenedChanged();
        // door is opened
        } else if (rsp.startsWith("data_received,5a,a5,0,5,a,a8,7f,7b")) {
            if (m_latestLightType == "uv_penetrate") {
                closeAllLight();
            }
            if (m_latestLightType == "blue_penetrate") {
                closeAllLight();
            }
            emit doorOpened();
        } else if (rsp.startsWith("data_received,5a,a5,0,5,a,a8,6f,6b"))
            emit doorClosed();
        else if (rsp.startsWith("data_received,5a,a5,0,6,a,a2")) {
            parsePowerOnTick(rsp);
        }
        qDebug() << "mcu rsp: " << rsp;
    }
}

void WzMcuQml::switchLight(const QString& lightType, const bool& isOpen) {
    if (m_openId == -1) {
        qInfo() << "Mcu, switchLight, open_id = -1";
        return;
    }

    qInfo() << "Mcu, switchLight, lightType:" << lightType << ", isOpen:" << isOpen;

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
            cmd = "5a,a5,00,04,03,67,55,55,aa,bb";
        } else if (lightType == "blue_penetrate_force") {
            cmd = "5a,a5,00,04,03,67,55,55,aa,bb";
        } else if (lightType == "red") {
            cmd = "5a,a5,00,04,03,7a,07,08,aa,bb";
            m_virtualCmdReplyTimer->start();
        } else if (lightType == "green") {
            cmd = "5a,a5,00,04,03,79,07,08,aa,bb";
            m_virtualCmdReplyTimer->start();
        } else if (lightType == "blue") {
            cmd = "5a,a5,00,04,03,78,07,08,aa,bb";
            m_virtualCmdReplyTimer->start();
        } else if (lightType == "uv_reflex1") {
            if (m_cachedIniCmd)
                cmd = m_cmdUvReflex1;
            else {
                WzIniSetting iniSetting;
                cmd = iniSetting.readStr("Cmd", "UvReflex1", "5a,a5,00,04,03,76,07,08,aa,bb");
            }
            m_virtualCmdReplyTimer->start();
        } else if (lightType == "uv_reflex2") {
            if (m_cachedIniCmd)
                cmd = m_cmdUvReflex2;
            else {
                WzIniSetting iniSetting;
                cmd = iniSetting.readStr("Cmd", "UvReflex2", "5a,a5,00,04,03,74,07,08,aa,bb");
            }
            m_virtualCmdReplyTimer->start();
        }
        m_latestLightOpened = true;
        emit latestLightOpenedChanged();
    } else {
        cmd = "5a,a5,00,02,03,75,aa,bb";
        m_latestLightOpened = false;
        emit latestLightOpenedChanged();
    }
    m_latestLightType = lightType;
    emit latestLightTypeChanged();
    cmd = QString("send_data,%1,%2").arg(m_openId).arg(cmd);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
}

void WzMcuQml::closeAllLight() {
    if (m_openId == -1) return;
    QString cmd;
    cmd = "5a,a5,00,02,03,75,aa,bb";
    cmd = QString("send_data,%1,%2").arg(m_openId).arg(cmd);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
}

void WzMcuQml::init() {
    if (nullptr == m_libusbThread) {
        m_libusbThread = new LibusbThread();
        connect(m_libusbThread, &LibusbThread::rsp, this, &WzMcuQml::rsp);
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

void WzMcuQml::getPowerOnTick()
{
    if (m_openId == -1) return;
    m_repeatSetFilterInternal = false;
    QString cmd = QString("send_data,%1,5a,a5,00,02,0a,a2,aa,bb").arg(m_openId);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
}

void WzMcuQml::exec(const QString &cmd)
{
    if (m_libusbThread)
        m_libusbThread->exec(cmd);
}

void WzMcuQml::openDevice(const int &bus, const int &address)
{
    QString cmd = QString("open_device,%1,%2").arg(bus).arg(address);
    m_libusbThread->exec(cmd);
}

void WzMcuQml::focusStartFar() {
    if (m_openId == -1) return;
    m_repeatSetFilterInternal = false;
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,65,%2,%3,aa,bb").arg(m_openId).arg("00", "00");
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

void WzMcuQml::focusStartNear() {
    if (m_openId == -1) return;
    m_repeatSetFilterInternal = false;
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,62,%2,%3,aa,bb").arg(m_openId).arg("00", "00");
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
    m_latestFilterCmd = cmd;
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

int WzMcuQml::getOpenId() const {
    return m_openId;
}
