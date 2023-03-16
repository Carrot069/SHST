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

void WzUtils::calc16bitTo8bit(uint16_t* buffer, uint32_t count, uint8_t* mapTable) {
    uint16_t* bufferPtr = buffer;
    uint16_t min = *bufferPtr;
    uint16_t max = *bufferPtr;
    for (uint32_t n = 0; n < count; n++) {
        if (min > *bufferPtr) min = *bufferPtr;
        if (max < *bufferPtr) max = *bufferPtr;
        bufferPtr++;
    }
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

bool WzUtils::isNoLogo() {
#ifdef NO_LOGO
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

QRect WzUtils::absRect(const QPoint &p1, const QPoint &p2)
{
    int x1 = qMin(p1.x(), p2.x());
    int y1 = qMin(p1.y(), p2.y());
    int x2 = qMax(p1.x(), p2.x());
    int y2 = qMax(p1.y(), p2.y());
    return QRect(x1, y1, x2 - x1, y2 - y1);
}

QRect WzUtils::absRect(const QRect &r)
{
    int x1 = qMin(r.left(), r.right());
    int y1 = qMin(r.top(), r.bottom());
    int x2 = qMax(r.left(), r.right());
    int y2 = qMax(r.top(), r.bottom());
    return QRect(x1, y1, x2 - x1, y2 - y1);
}

void WzUtils::absPoints(QPoint &p1, QPoint &p2)
{
    int x1 = qMin(p1.x(), p2.x());
    int y1 = qMin(p1.y(), p2.y());
    int x2 = qMax(p1.x(), p2.x());
    int y2 = qMax(p1.y(), p2.y());

    p1.setX(x1);
    p1.setY(y1);
    p2.setX(x2);
    p2.setY(y2);
}

double WzUtils::getAngle(const QPoint &p1, const QPoint &p2)
{
    double dltA;
    int dltx, dlty;
    double cc;

    dltx = p2.x() - p1.x();
    dlty = p2.y() - p1.y();

    if (dltx != 0)
        dltA = atan2(dlty, dltx);
    else
        dltA = 3.1415926 / 2;

    cc = abs(dltA);
    if (cc > 1.5707963) {
        if (dltA > 0)
            dltA = dltA - 3.1415926;
        else
            dltA = dltA + 3.1415926;
    }

    return dltA;
}

void WzUtils::RC(uint16_t* buf, int width, int height, int x, int y, uint16_t &rgb, const uint16_t &defaultColor)
{
    uint16_t* cc;

    if (x <= width - 1 && x >= 0 && y <= height - 1 && y >= 0) {
        cc = buf;
        cc += y * width + x;
        rgb = *cc;
    } else {
        rgb = defaultColor;
    }
}
void WzUtils::RC(uint8_t* buf, int width, int height, int x, int y, uint8_t &rgb, const uint8_t &defaultColor)
{
    uint8_t* cc;

    if (x <= width - 1 && x >= 0 && y <= height - 1 && y >= 0) {
        cc = buf;
        cc += y * width + x;
        rgb = *cc;
    } else {
        rgb = defaultColor;
    }
}
uint16_t WzUtils::Bilinear(uint16_t* buf, int width, int height, double x, double y, const uint16_t &defaultColor)
{
    int j, k, rr;
    double cx, cy, m0, m1;
    uint16_t p0, p1, p2, p3;

    j = trunc(x);
    k = trunc(y);
    cx = x - floor(x);
    cy = y - floor(y);
    RC(buf, width, height, j, k, p0, defaultColor);

    RC(buf, width, height, j, k, p0, defaultColor);
    RC(buf, width, height, j + 1, k, p1, defaultColor);
    RC(buf, width, height, j, k + 1, p2, defaultColor);
    RC(buf, width, height, j + 1, k + 1, p3, defaultColor);
    m0 = p0 + cx * (p1 - p0);
    m1 = p2 + cx * (p3 - p2);
    rr = trunc(m0 + cy * (m1 - m0));
    return rr;
}
uint8_t WzUtils::Bilinear(uint8_t* buf, int width, int height, double x, double y, const uint8_t &defaultColor)
{
    int j, k, rr;
    double cx, cy, m0, m1;
    uint8_t p0, p1, p2, p3;

    j = trunc(x);
    k = trunc(y);
    cx = x - floor(x);
    cy = y - floor(y);
    RC(buf, width, height, j, k, p0, defaultColor);

    RC(buf, width, height, j, k, p0, defaultColor);
    RC(buf, width, height, j + 1, k, p1, defaultColor);
    RC(buf, width, height, j, k + 1, p2, defaultColor);
    RC(buf, width, height, j + 1, k + 1, p3, defaultColor);
    m0 = p0 + cx * (p1 - p0);
    m1 = p2 + cx * (p3 - p2);
    rr = trunc(m0 + cy * (m1 - m0));
    return rr;
}

QRect WzUtils::zoomRect(const QRect &rect, const qreal zoom)
{
    QRect r;
    r.setX(static_cast<qreal>(rect.x()) * zoom / 100.0);
    r.setY(static_cast<qreal>(rect.y()) * zoom / 100.0);
    r.setWidth(static_cast<qreal>(rect.width()) * zoom / 100.0);
    r.setHeight(static_cast<qreal>(rect.height()) * zoom / 100.0);
    return r;
}

QPoint WzUtils::zoomPoint(const QPoint &point, const qreal zoom)
{
    QPoint p;
    p.setX(static_cast<qreal>(point.x()) * zoom / 100.0);
    p.setY(static_cast<qreal>(point.y()) * zoom / 100.0);
    return p;
}

QPoint WzUtils::noZoomPoint(const QPoint &point, const qreal zoom)
{
    QPoint p;
    p.setX(static_cast<qreal>(point.x() * 100) / zoom);
    p.setY(static_cast<qreal>(point.y() * 100) / zoom);
    return p;
}
