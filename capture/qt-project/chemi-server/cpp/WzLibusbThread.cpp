#include "WzLibusbThread.h"

static void LIBUSB_CALL xfer_callback(struct libusb_transfer *transfer) {
    LibusbThread* thread = reinterpret_cast<LibusbThread*>(transfer->user_data);
    bool isContinueRead = thread->dataReceived(transfer);
    // Prepare and re-submit the read request.
    if (isContinueRead)
        libusb_submit_transfer(transfer);
}

void LibusbHandleEventsThread::run() {
    struct timeval timeout;

    // Use a 1 second timeout for the libusb_handle_events_timeout call
    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;

    // Keep handling events until transfer stop is requested.
    do {
        if (m_isHandleEvents) {
            libusb_handle_events_timeout(nullptr, &timeout);
        } else {
            QThread::msleep(100);
        }
    } while (!m_isStop);
}

void LibusbHandleEventsThread::stopThread() {
    m_isStop = true;
}

void LibusbHandleEventsThread::stopHandle() {
    m_isHandleEvents = false;
    QThread::msleep(1100); // 这个时间要比 run 函数中调用 libusb_handle_events_timeout 阻塞的时间略长
}

void LibusbHandleEventsThread::startHandle() {
    m_isHandleEvents = true;
}

LibusbHandleEventsThread::LibusbHandleEventsThread(QObject *parent): QThread(parent) {
    qDebug() << "LibusbHandleEventsThread created";
}

LibusbHandleEventsThread::~LibusbHandleEventsThread() {
    qDebug() << "~LibusbHandleEventsThread";
}

LibusbThread::LibusbThread(QObject *parent):
    QThread(parent),
    m_openDeviceIndex(0),
    m_busy(true),
    m_initCount(0)
{
    qDebug() << "LibusbThread created";
    connect(this, &LibusbThread::finished, this, &QObject::deleteLater);
}

LibusbThread::~LibusbThread() {
    qDebug() << "~LibusbThread";
}

void LibusbThread::run() {
    QString action;
    LibusbHandleEventsThread* handleEventsThread;
    forever {
        m_mutex.lock();
        if (m_actions.count() == 0) {
            m_busy = false;
            m_newAction.wait(&m_mutex);
        }
        action = m_actions[0];
        qDebug() << "LibusbThread::run, action: " << action;
        m_actions.removeAt(0);
        m_mutex.unlock();

        if(action == "libusb_init") {
            if (m_initCount == 0) {
                m_initCount++;
                libusb_init(nullptr);
                handleEventsThread = new LibusbHandleEventsThread();
                handleEventsThread->start();
                handleEventsThread->startHandle();
            } else {
                m_initCount++;
            }        
            emit rsp(action);
        } else if (action == "libusb_uninit") {
            if (m_initCount > 0) {
                m_initCount--;
                if (m_initCount == 0) {
                    handleEventsThread->stopHandle();
                    handleEventsThread->stopThread();
                    handleEventsThread->wait();
                    delete handleEventsThread;
                    handleEventsThread = nullptr;
                    libusb_exit(nullptr);
                }
            }
            emit rsp(action);
        } else if (action.startsWith("enum_devices")) {
            QString results = enumDevices(action);
            emit rsp("enum_devices," + results);
        } else if (action.startsWith("open_device")) {
            QString results = openDevice(action);
            emit rsp("open_device," + results);
        } else if (action.startsWith("send_data")) {
            QString results = sendData(action);
            emit rsp("send_data," + results);
        } else if (action.startsWith("close_device")) {
            QString results = closeDevice(action);
            emit rsp("close_device," + results);
        } else if (action == "exit_thread") {
            break;
        }
    }

    qDebug() << "LibusbThread::run finished";
}

void LibusbThread::exec(const QString& action) {
    m_mutex.lock();
    m_actions.append(action);
    m_busy = true;
    m_newAction.wakeOne();
    m_mutex.unlock();
}

bool LibusbThread::isBusy() {
    QMutexLocker locker(&m_mutex);
    return m_busy;
}

void LibusbThread::stop() {
    exec("exit_thread");
}

void LibusbThread::enumDevices(const int *vid, const int *pid, const int &idCount, QVariantMap &results) {
    libusb_device **devs = nullptr; // USB devices
    ssize_t cnt = 0;                // USB device count

    //Detect all devices on the USB bus.
    cnt = libusb_get_device_list(nullptr, &devs);
    if (cnt < 0) {
        results["error_number"] = static_cast<int>(cnt);
        results["error_msg"] = libusb_error_name(static_cast<int>(cnt));
        results["count"] = 0;
        return;
    }

    QList<QVariant> devices;

    int i = 0;
    int err = 0;
    int deviceCount = 0;

    libusb_device* device = nullptr;
    while ((device = devs[i++]) != nullptr)
    {
        struct libusb_device_descriptor desc;

        err = libusb_get_device_descriptor(device, &desc);
        if (err) {
            results["error_number"] = static_cast<int>(cnt);
            results["error_msg"] = libusb_error_name(static_cast<int>(cnt));
            return;
        }

        bool isEqual = false;
        if (idCount == 0)
            isEqual = true;
        else {
            for (int n = 0; n < idCount; n++) {
                if (desc.idVendor == vid[n] && desc.idProduct == pid[n]) {
                    isEqual = true;
                    break;
                }
            }
        }
        if (isEqual) {
            QVariantMap deviceMap;
            deviceMap["vid"] = desc.idVendor;
            deviceMap["pid"] = desc.idProduct;
            deviceMap["bus"] = libusb_get_bus_number(device);
            deviceMap["address"] = libusb_get_device_address(device);
            deviceCount++;
            devices.append(deviceMap);
        }
    }

    results["count"] = deviceCount;
    results["devices"] = devices;

    libusb_free_device_list(devs, 1);
}

QString LibusbThread::enumDevices(const QString& action) {
    QVector<int> vid, pid;
    if (action.contains(',')) {
        QStringList sl = action.split(',');
        for (int n = 1; n < sl.count() - 1; n++) {
            if (n % 2 == 1) {
                vid.append(sl[n].toInt());
                pid.append(sl[n+1].toInt());
            }
        }
    } else {
    }
    QVariantMap results;
    LibusbThread::enumDevices(vid.data(), pid.data(), vid.count(), results);
    QJsonDocument json = QJsonDocument::fromVariant(results);
    return json.toJson();
}

QString LibusbThread::openDevice(const QString& action) {
    QVariantMap results;
    QStringList sl = action.split(',');
    if (sl.count() < 3) {
        return errorToJsonStr(NOT_ENOUGH_PARAMETER, "not enough parameter");
    }

    libusb_device **devs = nullptr; // USB devices
    ssize_t cnt = 0;                // USB device count

    //Detect all devices on the USB bus.
    cnt = libusb_get_device_list(nullptr, &devs);
    if (cnt < 0) {
        if (devs) libusb_free_device_list(devs, 1);
        return libusbErrorToJsonStr(static_cast<int>(cnt));
    }

    int i = 0;
    int err = 0;
    int bus_number = sl[1].toInt();
    int address = sl[2].toInt();

    libusb_device* device = nullptr;
    bool isFound = false;
    while ((device = devs[i++]) != nullptr)
    {
        struct libusb_device_descriptor desc;

        err = libusb_get_device_descriptor(device, &desc);
        if (err < 0) {
            if (devs) libusb_free_device_list(devs, 1);
            return libusbErrorToJsonStr(err);
        }

        if (libusb_get_bus_number(device) == bus_number &&
                libusb_get_device_address(device) == address)
        {
            isFound = true;
            break;
        }
    }

    if (!isFound) {
        if (devs) libusb_free_device_list(devs, 1);
        return libusbErrorToJsonStr(LIBUSB_ERROR_NO_DEVICE);
    }

    libusb_device_handle *handle = nullptr;
    err = libusb_open(device, &handle);
    if (err < 0) {
        if (devs) libusb_free_device_list(devs, 1);
        return libusbErrorToJsonStr(err);
    }

    libusb_free_device_list(devs, 1);

    err = libusb_claim_interface(handle, DEFAULT_INTERFACE_NUMBER);
    if (err) {
        libusb_close(handle);
        return libusbErrorToJsonStr(err);
    }

    UsbDevice* usbDevice = new UsbDevice();
    usbDevice->handle = handle;
    usbDevice->endPointReading = END_POINT_READ;

    m_openedDevices[m_openDeviceIndex] = usbDevice;
    results["open_id"] = m_openDeviceIndex;
    m_openDeviceIndex++;

    submitTransfer(usbDevice);

    QJsonDocument json = QJsonDocument::fromVariant(results);
    return json.toJson();
}

QString LibusbThread::closeDevice(const QString& action) {
    QVariantMap results;
    QStringList sl = action.split(',');
    if (sl.count() < 2) {
        return errorToJsonStr(NOT_ENOUGH_PARAMETER, "not enough parameter");
    }

    uint32_t openId = sl[1].toUInt();
    if (m_openedDevices.contains(openId)) {
        UsbDevice *device = m_openedDevices[openId];
        libusb_release_interface(device->handle, DEFAULT_INTERFACE_NUMBER);
        libusb_close(device->handle);
        results["close_id"] = openId;
        if (device->buffer) {
            delete[] device->buffer;
            device->buffer = nullptr;
        }
        if (device->transfer) {
            libusb_free_transfer(device->transfer);
            device->transfer = nullptr;
        }
        delete device;
    }

    QJsonDocument json = QJsonDocument::fromVariant(results);
    return json.toJson();
}

QString LibusbThread::sendData(const QString &action) {
    QVariantMap results;
    QStringList sl = action.split(',');
    if (sl.count() < 3) {
        return errorToJsonStr(NOT_ENOUGH_PARAMETER, "not enough parameter");
    }

    uint32_t openId = sl[1].toUInt();
    if (!m_openedDevices.contains(openId)) {
        return errorToJsonStr(DEVICE_NOT_OPEN, "device not open");
    }

    UsbDevice* device = m_openedDevices[openId];
    libusb_device_handle *handle = device->handle;

    std::vector<unsigned char> buffer(static_cast<size_t>(sl.count() - 2));
    for(int n = 2; n < sl.count(); n++) {
        buffer.at(static_cast<size_t>(n-2)) = (sl[n].toUShort(nullptr, 16) & 0xff);
    }

    int dataLength = static_cast<int>(buffer.size());
    int actualLength = 0;
    int err = libusb_bulk_transfer(handle, END_POINT_WRITE,
                               buffer.data(), dataLength, &actualLength, 2000);
    if (err) {
        return libusbErrorToJsonStr(err);
    }

    return "{}";
}

QString LibusbThread::libusbErrorToJsonStr(int err) {
    QString errorMsg = libusb_error_name(err);
    return errorToJsonStr(err, errorMsg);
}

QString LibusbThread::errorToJsonStr(int err, const QString &msg) {
    QVariantMap results;
    results["error_number"] = err;
    results["error_msg"] = msg;
    QJsonDocument json = QJsonDocument::fromVariant(results);
    return json.toJson();
}

int LibusbThread::submitTransfer(UsbDevice *device) {
    device->transfer = libusb_alloc_transfer(0);
    if (!device->transfer) {
        return LIBUSB_ALLOC_TRANSFER_NULL;
    }

    if (device->buffer == nullptr) {
        device->buffer = new unsigned char[device->bufferSize];
    }

    libusb_fill_bulk_transfer(device->transfer,
                              device->handle,
                              device->endPointReading,
                              device->buffer,
                              device->bufferSize,
                              xfer_callback,
                              this,
                              device->readTimeout);

    int ret = libusb_submit_transfer(device->transfer);
    if (ret) {
        delete[] device->buffer;
        return ret;
    }

    return LIBUSB_SUCCESS;
}

// 被 libusb 回调函数调用, 返回 true 表示继续读取数据, 否则停止
bool LibusbThread::dataReceived(struct libusb_transfer *transfer) {
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        // TODO 记录这个错误信息        
    } else {
        QString data;
        for(int n = 0; n < transfer->actual_length; n++) {
            if (n>0) data.append(',');
            data.append(QString::number(transfer->buffer[n], 16));
        }
        emit rsp("data_received," + data);
    }
    return true;
}
