#include "WzRenderThread.h"

#ifdef HARDLOCK
QString WzRenderThread::sn;
#endif

WzRenderThread::WzRenderThread(QObject *parent)
    : QThread(parent)
{
}

WzRenderThread::~WzRenderThread() {
    if (!m_abort)
        m_abort = true;
    wait();
}

void WzRenderThread::run() {
    while (!m_abort) {
        // 30 ~ 90 秒检测一次是否有加密锁
        QRandomGenerator rg(static_cast<quint32>(QDateTime::currentMSecsSinceEpoch()));
        int waitSeconds = rg.bounded(30, 90);
        qDebug("WzRenderThread, waitSeconds(1): %d", waitSeconds);

        for (int i = 0; i < waitSeconds; i++) {
            for (int j = 0; j < 100; j++) {
                if (m_abort)
                    return;
                QThread::msleep(10);
            }
        }

        if (getCount() > 0)
            continue;

        QRandomGenerator rg2(static_cast<quint32>(QDateTime::currentMSecsSinceEpoch()));
        waitSeconds = rg2.bounded(60, 120);
        qDebug("WzRenderThread, waitSeconds(2): %d", waitSeconds);

        for (int i = 0; i < waitSeconds; i++) {
            for (int j = 0; j < 100; j++) {
                if (m_abort)
                    return;
                QThread::msleep(10);
            }
        }

        if (getCount() > 0)
            continue;

        emit render();
    }
}

int WzRenderThread::getCount() {
#ifdef HARDLOCK
    unsigned char v1 = 0x34;
    unsigned char v2 = 0x32;
    unsigned char v3 = 0x38;
    unsigned char v4 = 0x30;
    unsigned char v5 = 0x35;
    unsigned char v6 = 0x39;
    unsigned char v7 = 0x32;
    unsigned char v8 = 0x38;

    char vid[9];
    vid[8] = 0;

    for (int i = 0; i < 9; i++) {
        switch (i) {
            case 0: vid[i] = v1; break;
            case 1: vid[i] = v2; break;
            case 2: vid[i] = v3; break;
            case 3: vid[i] = v4; break;
            case 4: vid[i] = v5; break;
            case 5: vid[i] = v6; break;
            case 6: vid[i] = v7; break;
            case 7: vid[i] = v8; break;
            case 8: vid[i] = 0; break;
        }
    }

    int count = 0;
    RY3_Find(vid, &count);
    return count;
#else
    return 1;
#endif
}

void WzRenderThread::startThread() {
    start();
}

void WzRenderThread::stopThread() {
    m_abort = true;
}

#ifdef HARDLOCK
unsigned long WzRenderThread::getHandle(const int count, RY_HANDLE* pHandle)
{
#ifdef HARDLOCK
    WzDatabaseService db;
    QString previousOpenedSN = db.readStrOption("ry3_sn", "");
    if (count == 1 || previousOpenedSN == "") {
        unsigned long ret = RY3_Open(pHandle, 1);
        if (RY3_SUCCESS != ret)
            return ret;
        char buf[255] = {0};
        ret = RY3_GetHardID(*pHandle, buf);
        if (RY3_SUCCESS != ret)
            return ret;
        WzRenderThread::sn = QString::fromLocal8Bit(buf);
        previousOpenedSN = WzRenderThread::sn;
        db.saveStrOption("ry3_sn", previousOpenedSN);
        return ret;
    } else {
        for (int i = 1; i <= count; i++) {
            unsigned long ret = RY3_Open(pHandle, i);
            if (RY3_SUCCESS != ret)
                continue;
            char buf[255] = {0};
            ret = RY3_GetHardID(*pHandle, buf);
            if (RY3_SUCCESS != ret) {
                RY3_Close(*pHandle, true);
                continue;
            }
            WzRenderThread::sn = QString::fromLocal8Bit(buf);
            if (previousOpenedSN == WzRenderThread::sn)
                return RY3_SUCCESS;
            else
                RY3_Close(*pHandle, true);
        }
    }

    return SHST_RY3_NOT_FOUND;
#endif
}

int WzRenderThread::getFirstSn(QString &sn)
{
#ifdef HARDLOCK
    unsigned char v1 = 0x34;
    unsigned char v2 = 0x32;
    unsigned char v3 = 0x38;
    unsigned char v4 = 0x30;
    unsigned char v5 = 0x35;
    unsigned char v6 = 0x39;
    unsigned char v7 = 0x32;
    unsigned char v8 = 0x38;

    char vid[9];
    vid[8] = 0;

    for (int i = 0; i < 9; i++) {
        switch (i) {
        case 0: vid[i] = v1; break;
        case 1: vid[i] = v2; break;
        case 2: vid[i] = v3; break;
        case 3: vid[i] = v4; break;
        case 4: vid[i] = v5; break;
        case 5: vid[i] = v6; break;
        case 6: vid[i] = v7; break;
        case 7: vid[i] = v8; break;
        case 8: vid[i] = 0; break;
        }
    }

    int count = 0;
    RY3_Find(vid, &count);
    if (count == 0)
        return SHST_RY3_NOT_FOUND;

    RY_HANDLE handle;
    unsigned long ret = RY3_Open(&handle, 1);
    if (RY3_SUCCESS != ret)
        return ret;

    char buf[255] = {0};
    ret = RY3_GetHardID(handle, buf);
    if (RY3_SUCCESS != ret) {
        RY3_Close(handle, true);
        return ret;
    }
    RY3_Close(handle, true);

    sn = QString::fromLocal8Bit(buf);

    return SHST_SUCCESS;
#else
    return SHST_NO_HARDLOCK;
#endif
}
#endif
