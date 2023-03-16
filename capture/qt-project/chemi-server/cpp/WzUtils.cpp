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
    qDebug() << "WzUtils created";
}

WzUtils::~WzUtils() {
    qDebug() << "~WzUtils";
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

bool WzUtils::isTcpClient()
{
#ifdef TCP_CLIENT
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

void WzUtils::openConfigPath()
{
#ifdef Q_OS_WIN
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    ShellExecuteW(0, L"open", path.toStdWString().c_str(), NULL, NULL, SW_SHOW);
#endif
}

void WzUtils::takeCrash()
{
    uint32_t* buf = new uint32_t[1000];
    for (int i = 0; i < 999; i++)
        buf[i * i] = i;
}
