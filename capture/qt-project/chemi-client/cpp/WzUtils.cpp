#include "WzUtils.h"

AutoFreeArray::AutoFreeArray(uint16_t* buf) {
    buf16bit = buf;
}
AutoFreeArray::~AutoFreeArray() {
    if (nullptr != buf16bit) {
        delete []buf16bit;
        buf16bit = nullptr;
    }
}

WzUtils::WzUtils(QObject* parent): QObject(parent) {
    qInfo() << "Utils created";
}

WzUtils::~WzUtils() {
    qInfo() << "~Utils";
}

QString WzUtils::getTimeStr(int ms, bool isShowMs, bool isText) {
    QTime t(0, 0, 0, 0);
    t = t.addMSecs(ms);
    QString str = "";

    if (isText) {
        if (!isShowMs && t.msec() >= 500)
            t = t.addMSecs(1000 - t.msec());
        if (t.minute() > 0)
            str += QString::number(t.minute()) + tr("分");
        if (t.second() > 0)
            str += QString::number(t.second()) + tr("秒");
        if (t.msec() > 0)
            str += QString::number(t.msec()) + tr("毫秒");
        return str;
    } else {
        if (isShowMs) {
            return str.sprintf("%.2d:%.2d.%.3d", t.minute(), t.second(), t.msec());
        } else {
            if (t.msec() >= 500)
                t = t.addMSecs(1000 - t.msec());
            return str.sprintf("%.2d:%.2d", t.minute(), t.second());
        }
    }
}

void WzUtils::calc16bitTo8bit(uint16_t* buffer, uint32_t count, uint8_t* mapTable, const int& maxLimit) {
    uint16_t* bufferPtr = buffer;
    uint16_t min = *bufferPtr;
    uint16_t max = *bufferPtr;
    for (uint32_t n = 0; n < count; n++) {
        if (min > *bufferPtr) min = *bufferPtr;
        if (max < *bufferPtr) max = *bufferPtr;
        bufferPtr++;
    }
    if (max < maxLimit)
        max = maxLimit;
    for (uint16_t n = 0; n < min; n++)
        mapTable[n] = 0;
    for (uint32_t n = max; n < 65536; n++)
        mapTable[n] = 255;
    double scale = (max - min) / 254;
    for (uint32_t n = min; n < max; n++) {
        double bit8 = (n - min) / scale;
        if (bit8 > 255) bit8 = 255;
        mapTable[n] = static_cast<uint8_t>(bit8);
    }
}

void WzUtils::processEvents() {
    qApp->processEvents(QEventLoop::AllEvents);
}

void WzUtils::copyToClipboard(const QString& text) {
    QClipboard* cp = QApplication::clipboard();
    cp->setText(text);
}

bool WzUtils::validatePath(const QString& path) {
    QDir p(path);
    return p.mkpath(".");
}

QString WzUtils::getFileNameWithPath(const QString &path, const QString &fileName)
{
    QUrl url(path);
    QDir dir(url.toLocalFile());
    QFileInfo fi(dir, fileName);
    qInfo() << Q_FUNC_INFO << ":" << fi.absoluteFilePath();
    return fi.absoluteFilePath();
}

QString WzUtils::toLocalPath(const QString &path) {
    QUrl url(path);
    return url.toLocalFile();
}

QString WzUtils::toLocalFile(const QString& fileName) {
    QUrl url(fileName);
    return url.toLocalFile();
}

QString WzUtils::appName() {
    return QApplication::applicationName();
}

QString WzUtils::appVersion() {
    return APP_VERSION;
}

bool WzUtils::isGelCapture() {
#ifdef GEL_CAPTURE
    return true;
#else
    return false;
#endif
}

bool WzUtils::isChemiCapture() {
#ifdef GEL_CAPTURE
    return false;
#else
    return true;
#endif
}

bool WzUtils::isPC()
{
#ifdef Q_OS_WIN
    return true;
#endif
#ifdef Q_OS_MACOS
    return true;
#endif
    return false;
}

bool WzUtils::isAndroid()
{
#ifdef Q_OS_ANDROID
    return true;
#else
    return false;
#endif
}

bool WzUtils::isPhone()
{
#ifdef PHONE
    return true;
#else
    return false;
#endif
}

bool WzUtils::isPad()
{
#ifdef PAD
    return true;
#else
    return false;
#endif
}

bool WzUtils::isTcpClient()
{
#ifdef TCP_CLIENT
    return true;
#else
    return false;
#endif
}

bool WzUtils::isDemo()
{
#ifdef DEMO
    return true;
#else
    return false;
#endif
}

bool WzUtils::isMobile()
{
#ifdef MOBILE
    return true;
#else
    return false;
#endif
}

bool WzUtils::aesDecryptFileToBuf(uint8_t *key, QString fileName, char **buffer, int* bufferSize) {
    using namespace std;
    ifstream inFile(fileName.toLocal8Bit(), ios::binary | ios::in | ios::ate);
    if (inFile.is_open()) {
        int fileSize = inFile.tellg();
        int bufSize = fileSize - IV_LEN;

        char* fileBuf = new char[bufSize];
        memset(fileBuf, 0, bufSize);

        inFile.seekg(0, ios::beg);
        inFile.read(fileBuf, bufSize);

        aes_iv iv;
        inFile.read(reinterpret_cast<char*>(&iv[0]), IV_LEN);

        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, key, iv);
        AES_CBC_decrypt_buffer(&ctx, reinterpret_cast<uint8_t*>(fileBuf), bufSize);

        unsigned char padSize = fileBuf[bufSize-1];

        *bufferSize = bufSize - padSize + 1;
        *buffer = new char[*bufferSize];
        memset(*buffer, 0, *bufferSize);
        memcpy(*buffer, fileBuf, *bufferSize-1);

        delete []fileBuf;
        inFile.close();

        return true;
    } else {
        qWarning() << "Unable open the file: " << fileName << endl;
        return false;
    }
}

QString WzUtils::aesEncryptStr(const aes256_key key, const QString &s)
{
    const int blockSize = 16;
    const QByteArray bytes = s.toUtf8();

    int fileSize = bytes.size();
    unsigned char padSize = blockSize - (fileSize % blockSize);
    if (padSize == 0)
        padSize = blockSize;
    int bufSize = fileSize + padSize;

    char* fileBuf = new char[bufSize];
    memset(fileBuf, padSize, bufSize);
    memcpy(fileBuf, bytes.data(), fileSize);

    aes_iv iv;
    QRandomGenerator rg;
    rg.fillRange(reinterpret_cast<uint*>(iv), 4);

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, reinterpret_cast<uint8_t*>(fileBuf), bufSize);

    QByteArray outBytes;
    outBytes.append(fileBuf, bufSize);
    outBytes.append(reinterpret_cast<char*>(iv), IV_LEN);

    delete []fileBuf;

    return outBytes.toBase64();
}

QString WzUtils::aesDecryptStr(const aes256_key key, const QString &s) {
    QByteArray inBytes = QByteArray::fromBase64(s.toUtf8());

    int fileSize = inBytes.size();
    int bufSize = fileSize - IV_LEN;

    char* fileBuf = new char[bufSize];
    memset(fileBuf, 0, bufSize);

    memcpy(fileBuf, inBytes.data(), bufSize);

    aes_iv iv;
    char *p = inBytes.data();
    p += bufSize;
    memcpy(reinterpret_cast<char*>(&iv[0]), p, IV_LEN);

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, reinterpret_cast<uint8_t*>(fileBuf), bufSize);

    unsigned char padSize = fileBuf[bufSize-1];

    int bufferSize = bufSize - padSize + 1;
    char *buffer = new char[bufferSize];
    memset(buffer, 0, bufferSize);
    memcpy(buffer, fileBuf, bufferSize-1);

    QString outStr(buffer);

    delete []fileBuf;
    delete []buffer;

    return outStr;
}

void WzUtils::sleep(int ms) {
    QElapsedTimer et;
    et.start();
    while (et.elapsed() < ms) {
        qApp->processEvents();
    }
}

QString WzUtils::desktopPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

bool WzUtils::fileExists(const QString &fileName)
{
    return QFileInfo(fileName).exists();
}


QRect WzUtils::fitRect(const QRect &outerRect, const QRect &innerRect,
                       double &scale,
                       const bool allowZoomOut, const bool allowZoomIn)
{
    QRect r;
    if (outerRect == innerRect) {
        scale = 100;
        return innerRect;
    // 内矩形完全小于外矩形
    } else if (outerRect.width() >= innerRect.width() &&
            outerRect.height() >= innerRect.height()) {
        // 允许放大
        if (allowZoomIn) {
            // 左右的空隙小于上下的空隙
            if (outerRect.width() - innerRect.width() < outerRect.height() - innerRect.height()) {
                scale = 100.0 + static_cast<double>((outerRect.width() - innerRect.width())) / innerRect.width() * 100;
                r.setLeft(0);
                r.setWidth(outerRect.width());
                r.setHeight(innerRect.height() * scale / 100.0);
                r.moveTop(static_cast<double>(outerRect.height() - r.height()) / 2);
            // 左右的空隙大于上下的空隙
            } else if (outerRect.width() - innerRect.width() > outerRect.height() - innerRect.height()) {
                scale = 100.0 + static_cast<double>((outerRect.height() - innerRect.height())) / innerRect.height() * 100;
                r.setTop(0);
                r.setHeight(outerRect.height());
                r.setWidth(innerRect.width() * scale / 100.0);
                r.moveLeft(static_cast<double>(outerRect.width() - r.width()) / 2);
            } else {
                scale = 100.0 + static_cast<double>((outerRect.height() - innerRect.height())) / innerRect.height() * 100;
                r.setTop(0);
                r.setHeight(outerRect.height());
                r.setWidth(outerRect.height());
                r.setLeft(0);
            }
        // 不允许放大, 直接居中内部矩形
        } else {
            scale = 100;
            r.setTop(static_cast<double>(outerRect.height() - innerRect.height()) / 2);
            r.setLeft(static_cast<double>(outerRect.width() - innerRect.width()) / 2);
            r.setWidth(innerRect.width());
            r.setHeight(innerRect.height());
        }
    // 内矩形的宽度或高度超过外矩形
    } else {
        // 允许缩小
        if (allowZoomOut) {
            // 超宽比超高的数字大
            if (outerRect.width() - innerRect.width() < outerRect.height() - innerRect.height()) {
                scale = 100.0 + static_cast<double>((outerRect.width() - innerRect.width())) / innerRect.width() * 100;
                r.setLeft(0);
                r.setWidth(outerRect.width());
                r.setHeight(innerRect.height() * scale / 100.0);
                r.moveTop(static_cast<double>(outerRect.height() - r.height()) / 2);
            // 超宽比超高的数字小
            } else if (outerRect.width() - innerRect.width() > outerRect.height() - innerRect.height()) {
                scale = 100.0 + static_cast<double>((outerRect.height() - innerRect.height())) / innerRect.height() * 100;
                r.setTop(0);
                r.setHeight(outerRect.height());
                r.setWidth(innerRect.width() * scale / 100.0);
                r.moveLeft(static_cast<double>(outerRect.width() - r.width()) / 2);
            } else {
                scale = 100.0 + static_cast<double>((outerRect.height() - innerRect.height())) / innerRect.height() * 100;
                r.setTop(0);
                r.setHeight(outerRect.height());
                r.setWidth(outerRect.height());
                r.setLeft(0);
            }
        // 不允许缩小, 直接居中内部矩形. 这种情况下内矩形的左边距或上边距是负数.
        } else {
            scale = 100;
            r.setTop(static_cast<double>(outerRect.height() - innerRect.height()) / 2);
            r.setLeft(static_cast<double>(outerRect.width() - innerRect.width()) / 2);
            r.setWidth(innerRect.width());
            r.setHeight(innerRect.height());
        }
    }

    return r;
}

void WzUtils::new8bitBuf(uint8_t **buf, int width, int height)
{
    if ((*buf) != nullptr) {
        delete [] (*buf);
    }
    *buf = new uint8_t[width * height];
}

void WzUtils::new16bitBuf(uint16_t **buf, int width, int height)
{
    if ((*buf) != nullptr) {
        delete [] (*buf);
    }
    *buf = new uint16_t[width * height];
}

void WzUtils::new32bitBuf(uint32_t **buf, int width, int height)
{
    if ((*buf) != nullptr) {
        delete [] (*buf);
    }
    *buf = new uint32_t[width * height];
}

void WzUtils::delete8bitBuf(uint8_t **buf)
{
    if (*(buf) != nullptr) {
        delete [] *(buf);
        *(buf) = nullptr;
    }
}

void WzUtils::delete16bitBuf(uint16_t **buf)
{
    if (*(buf) != nullptr) {
        delete [] *(buf);
        *(buf) = nullptr;
    }
}

void WzUtils::delete32bitBuf(uint32_t **buf)
{
    if (*(buf) != nullptr) {
        delete [] *(buf);
        *(buf) = nullptr;
    }
}

uint8_t *WzUtils::bit8BufExpand24(const uint8_t *buf, int w, int h)
{
    uint8_t* newBuf = new uint8_t[w * h * 3];
    uint8_t* pBuf = newBuf;
    for (int i = 0; i < w * h; i++) {
        *pBuf = buf[i];
        pBuf++;
        *pBuf = buf[i];
        pBuf++;
        *pBuf = buf[i];
        pBuf++;
    }
    return newBuf;
}
