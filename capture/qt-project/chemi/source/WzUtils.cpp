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
            return str.asprintf("%.2d:%.2d.%.3d", t.minute() + t.hour() * 60, t.second(), t.msec());
        } else {
            if (t.msec() >= 500)
                t = t.addMSecs(1000 - t.msec());
            return str.asprintf("%.2d:%.2d", t.minute() + t.hour() * 60, t.second());
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
    return p.mkpath(".");//判断上一级目录
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

bool WzUtils::isOnlyChinese()
{
#ifdef ONLY_CHINESE
    return true;
#else
    return false;
#endif
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

bool WzUtils::isDemo()
{
#ifdef DEMO
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

bool WzUtils::deleteFile(const QString &fileName)
{
    QDir dir;
    return dir.remove(fileName);
}

void WzUtils::openPath(const QString &path)
{
#ifdef WIN32
    std::wstring path2 = path.toStdWString();
    ShellExecute(0, L"open", path2.c_str(), 0, 0, SW_SHOW);
#else
    Q_UNUSED(path)
#endif
}

void WzUtils::openPathByFileName(const QString &fileName)
{
#ifdef WIN32
    QUrl url(fileName);
    QFileInfo fi(url.toLocalFile());
    QString path = fi.absolutePath();
    std::wstring path2 = path.toStdWString();
    ShellExecute(0, L"open", path2.c_str(), 0, 0, SW_SHOW);
#else
    Q_UNUSED(fileName)
#endif
}

void WzUtils::openDataPath()
{
    openPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

void WzUtils::openDbgView()
{
    openPath(QApplication::applicationDirPath() + "/dbgview.exe");
}

bool WzUtils::isExposureArea()
{
#ifdef EXPOSURE_AREA
    return true;
#else
    return false;
#endif
}

bool WzUtils::isMac()
{
#ifdef MAC
    return true;
#else
    return false;
#endif
}

bool WzUtils::isOEM()
{
#ifdef OEM
    return true;
#else
    return false;
#endif
}

bool WzUtils::isOEMJP()
{
#ifdef OEM_JP
    return true;
#else
    return false;
#endif
}

bool WzUtils::isNoLogo()
{
#ifdef NO_LOGO
    return true;
#else
    return false;
#endif
}

bool WzUtils::isMini()
{
#ifdef MINI
    return true;
#else
    return false;
#endif
}

bool WzUtils::isMedFilTiff8bit()
{
#ifdef MED_FIL_TIFF8
    return true;
#else
    return false;
#endif
}

QString WzUtils::lightTypeStr(const WzEnum::LightType &lt)
{
    switch(lt) {
    case WzEnum::Light_None:
        return "";
    case WzEnum::Light_WhiteUp:
        return "white_up";
    case WzEnum::Light_UvReflex:
        return "uv_reflex";
    case WzEnum::Light_WhiteDown:
        return "white_down";
    case WzEnum::Light_UvPenetrate:
        return "uv_penetrate";
    case WzEnum::Light_Red:
        return "red";
    case WzEnum::Light_Green:
        return "green";
    case WzEnum::Light_Blue:
        return "blue";
    case WzEnum::Light_UvReflex1:
        return "uv_reflex1";
    case WzEnum::Light_UvReflex2:
        return "uv_reflex2";
    case WzEnum::Light_BluePenetrate:
        return "blue_penetrate";
    }
}

WzEnum::LightType WzUtils::lightTypeFromStr(const QString &s)
{
    for (int i = WzEnum::Light_First; i <= WzEnum::Light_Last; i++) {
        WzEnum::LightType lt = static_cast<WzEnum::LightType>(i);
        if (s == lightTypeStr(lt))
            return lt;
    }
    return WzEnum::Light_None;
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
