#include "WzMcuQml.h"

QString WzMcuQml::getLatestLightType() {
    return m_latestLightType;
}

bool WzMcuQml::getLatestLightOpened() {
    return m_latestLightOpened;
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
    QString vidPid = db.readStrOption("vid_pid", "59A1,0834");
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

bool WzMcuQml::getConnected() const
{
    return m_connected;
}

WzMcuQml::WzMcuQml(QObject *parent) : QObject(parent) {
    qDebug() << "WzMcuQml created";
    m_focusStopTimer = new QTimer(this);
    m_focusStopTimer->setInterval(100);
    m_focusStopTimer->setSingleShot(false);
    connect(m_focusStopTimer, SIGNAL(timeout()), this, SLOT(focusStopTimerTimeout()));
    connect(&m_focusTimer, &QTimer::timeout, this, &WzMcuQml::focusTimerTimeout);
}

WzMcuQml::~WzMcuQml() {
    m_focusTimer.stop();
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
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,64,%2,%3,aa,bb").
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
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,63,%2,%3,aa,bb").
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

void WzMcuQml::rsp(const QString& rsp) {
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
#ifndef DEMO
        emit devicesChanged();
#endif
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
        if (rsp.startsWith("data_received,5a,a5,0,5,3,68,"))
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
            emit doorOpened();
        } else if (rsp.startsWith("data_received,5a,a5,0,5,a,a8,6f,6b"))
            emit doorClosed();
        qDebug() << "mcu rsp: " << rsp;
    }
}

void WzMcuQml::focusTimerTimeout()
{
    m_libusbThread->exec(m_focusCmd);
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
        } else if (lightType == "green") {
            cmd = "5a,a5,00,04,03,79,07,08,aa,bb";
        } else if (lightType == "blue") {
            cmd = "5a,a5,00,04,03,78,07,08,aa,bb";
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

void WzMcuQml::openDevice(const int &bus, const int &address)
{
    QString cmd = QString("open_device,%1,%2").arg(bus).arg(address);
    m_libusbThread->exec(cmd);
}

void WzMcuQml::focusStartFar() {
    if (m_openId == -1) return;
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,64,%2,%3,aa,bb").arg(m_openId).arg("00", "00");
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

void WzMcuQml::focusStartNear() {
    if (m_openId == -1) return;
    QString cmd = QString("send_data,%1,5a,a5,00,04,01,63,%2,%3,aa,bb").arg(m_openId).arg("00", "00");
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
}

void WzMcuQml::focusStartFar2(const qreal clickPos)
{
    qDebug() << "clickPos:" << clickPos;
    if (m_openId == -1) return;
    m_lowSpeedFocus = false;
    QString cmd;
    if (clickPos > 0.33 && clickPos < 0.66) {
        m_lowSpeedFocus = true;
        cmd = QString("send_data,%1,5a,a5,00,0a,01,64,00,00,64,c7,c7,00,c7,00,aa,bb").arg(m_openId);
        m_focusCmd = cmd;
        m_focusTimer.setSingleShot(false);
        m_focusTimer.start(230);
        return;
    } else if (clickPos < 0.33)
        cmd = QString("send_data,%1,5a,a5,00,0a,01,64,00,00,00,e5,02,00,c7,00,aa,bb").arg(m_openId);
    else
        cmd = QString("send_data,%1,5a,a5,00,0a,01,64,00,00,00,3b,02,00,c7,00,aa,bb").arg(m_openId);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
}

void WzMcuQml::focusStartNear2(const qreal clickPos)
{
    qDebug() << "clickPos:" << clickPos;
    if (m_openId == -1) return;
    m_lowSpeedFocus = false;
    QString cmd;
    if (clickPos > 0.33 && clickPos < 0.66) {
        m_lowSpeedFocus = true;
        cmd = QString("send_data,%1,5a,a5,00,0a,01,63,00,00,64,e5,c7,00,c7,00,aa,bb").arg(m_openId);
        m_focusCmd = cmd;
        m_focusTimer.setSingleShot(false);
        m_focusTimer.start(230);
        return;
    } else if (clickPos < 0.33)
        cmd = QString("send_data,%1,5a,a5,00,0a,01,63,00,00,00,db,02,00,c7,00,aa,bb").arg(m_openId);
    else
        cmd = QString("send_data,%1,5a,a5,00,0a,01,63,00,00,00,3b,02,00,c7,00,aa,bb").arg(m_openId);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};

void WzMcuQml::focusStop() {
    if (m_openId == -1) return;
    if (m_lowSpeedFocus) {
        m_focusTimer.stop();
        return;
    }
    QString cmd = QString("send_data,%1,5a,a5,00,02,01,67,aa,bb").arg(m_openId);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
    m_focusStopCount = 0;
    m_focusStopTimer->start();
};

void WzMcuQml::switchAperture(const int& apertureIndex) {
    if (m_openId == -1) return;
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
    QString filterIndexs[] = {"6c", "6d", "6e", "6f", "70", "71", "72", "73"};
    QString filterIndexStr = filterIndexs[filterIndex % 8];
    QString cmd = QString("send_data,%1,5a,a5,00,02,02,%3,aa,bb").arg(m_openId).arg(filterIndexStr);
    qDebug() << cmd;
    m_libusbThread->exec(cmd);
};
