#ifndef LIBUSBTHREAD_H
#define LIBUSBTHREAD_H

#include <QObject>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QDebug>
#include <QtGlobal>
#include <QJsonDocument>

#include <atomic>

#include "libusb.h"

#include "WzSingleton.h"

#define NOT_ENOUGH_PARAMETER        -1000
#define DEVICE_NOT_OPEN             -1001
#define LIBUSB_ALLOC_TRANSFER_NULL  -1002

#define DEFAULT_INTERFACE_NUMBER 1
#define END_POINT_READ  0x81
#define END_POINT_WRITE 0x03

class UsbDevice {
public:
    struct libusb_device_handle *handle = nullptr;
    struct libusb_transfer *transfer = nullptr;
    unsigned char endPointReading;
    int bufferSize = 1024; // 缓冲区大小
    unsigned char* buffer = nullptr; // 从usb设备读取的数据放在这个缓冲区
    uint32_t readTimeout = 2000;
};

class LibusbHandleEventsThread : public QThread
{    
private:
    std::atomic_bool m_isStop{false};
    std::atomic_bool m_isHandleEvents{false};
protected:
    void run() override;
public:
    void stopThread();
    void stopHandle();
    void startHandle();
    LibusbHandleEventsThread(QObject *parent = nullptr);
    ~LibusbHandleEventsThread() override;
};

class LibusbThread : public QThread
{
    Q_OBJECT
private:
    uint32_t m_openDeviceIndex;
    QMap<uint32_t, UsbDevice*> m_openedDevices;
    QWaitCondition m_newAction;
    QStringList m_actions;
    QMutex m_mutex;
    bool m_busy;
    uint32_t m_initCount; // 初始化次数, 用于防止重复初始化和正确的反初始化
    LibusbHandleEventsThread *m_readThread;

    // enum_devices,vid1,pid1,vid2,pid2,vidN,pidN
    QString enumDevices(const QString& action);

    // open_device,bus,address
    // {error_number: 0, error_msg: "", open_id: 0}
    QString openDevice(const QString& action);

    // action: close_device,open_id
    // result: {error_number: 0, error_msg: "", close_id: 0}
    QString closeDevice(const QString& action);

    // action: send_data,open_id,hex,hex,hex,...
    // result: {error_number: 0, error_msg: ""}
    QString sendData(const QString& action);

    // 提交异步传输请求
    int submitTransfer(UsbDevice *device);
protected:
    void run() override;
public:
    LibusbThread(QObject *parent = nullptr);
    ~LibusbThread() override;
    void exec(const QString& action);
    bool isBusy();
    void stop();

    // 被 libusb 回调函数调用, 返回 true 表示继续读取数据, 否则停止
    bool dataReceived(struct libusb_transfer *transfer);

    /* {
     *   error_number: 0,
     *   error_msg: "",
     *   count: 0,
     *   devices: [
     *     {
     *       vid: int,
     *       pid: int,
     *       bus: int,
     *       address: int
     *     }
     *   ]
     * }
     */
    static void enumDevices(const int* vid, const int* pid, const int& idCount, QVariantMap& results);

    static QString libusbErrorToJsonStr(int err);
    static QString errorToJsonStr(int err, const QString& msg);
signals:
    void rsp(const QString& rsp);
    void execBefore(const QString &cmd);
};

#endif // LIBUSBTHREAD_H
