#include "WzLiveImageView.h"

LiveImageView* g_liveImageView = nullptr;

LiveImageView::LiveImageView(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_image(new QImage(800, 600, QImage::Format_Indexed8)),
      m_colorTable()
{
    m_showWidth = m_image->width() * m_zoom / 100;
    m_showHeight = m_image->height() * m_zoom / 100;

    for(int i = 0; i < 256; ++i) m_colorTable.append(qRgb(i,i,i));
    m_image->setColorTable(m_colorTable);

    void* pImageData = m_image->bits();
    memset(pImageData, 0x7f, static_cast<size_t>(m_image->width() * m_image->height()));

    g_liveImageView = this;

#ifdef TCP_SERVER
    m_tcpServer = new WzTcpServer();
    m_tcpServer->listen(QHostAddress::Any, 60051);
#endif
#ifdef TCP_CLIENT
    m_tcpSocket = new QTcpSocket(this);
    m_imageStream.setDevice(m_tcpSocket);
    m_imageStream.setVersion(QDataStream::Qt_5_12);

    connect(m_tcpSocket, &QIODevice::readyRead, this, &LiveImageView::readImageFromTcpServer);
    connect(m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &LiveImageView::displaySocketError);
#endif
}

LiveImageView::~LiveImageView()
{
#ifdef TCP_SERVER
    delete m_tcpServer;
#endif
#ifdef TCP_CLIENT
    disconnectServer();
#endif
    g_liveImageView = nullptr;
    delete m_image;
}

void LiveImageView::paint(QPainter *painter)
{
    if (WzUtils::isMini() && !m_hasRealImage)
        return;

    QRect dstRect(0, 0, 0, 0);
    QRect srcRect(0, 0, m_image->width(), m_image->height());
    if (m_zoomEnabled) {
        int scaleX, scaleY;
        if (m_showWidth < this->width()) {
            dstRect.setLeft((this->width() - m_showWidth) / 2);
            dstRect.setWidth(m_showWidth);
        } else {
            dstRect.setLeft(0);
            dstRect.setWidth(this->width());
            scaleX = m_showWidth * 100 / this->width();
            srcRect.setLeft(m_image->width() * 100 / (m_showWidth * 100 / this->x()));
            srcRect.setWidth(m_image->width() * 100 / scaleX);
        }
        if (m_showHeight < this->height()) {
            dstRect.setTop((this->height() - m_showHeight) / 2);
            dstRect.setHeight(m_showHeight);
        } else {
            dstRect.setTop(0);
            dstRect.setHeight(this->height());
            scaleY = m_showHeight * 100 / this->height();
            srcRect.setTop(m_image->height() * 100 / (m_showHeight * 100 / this->y()));
            srcRect.setHeight(m_image->height() * 100 / scaleY);
        }
    } else {
        double scale;
        dstRect = WzUtils::fitRect(QRect(0, 0, width(), height()), m_image->rect(), scale, true, false);
        m_zoom = trunc(scale);
    }

    painter->drawImage(dstRect, *m_image, srcRect);
    m_imageSrcRect = srcRect;
    m_paintDstRect = dstRect;
}

void LiveImageView::updateImage(const uchar* imageData, const QSize& imageSize)
{
    if (imageSize.width() != m_image->width() ||
            imageSize.height() != m_image->height()) {
        delete m_image;
        m_image = new QImage(imageSize, QImage::Format_Indexed8);
        m_image->setColorTable(m_colorTable);
        m_image->fill(Qt::white);
        showSizeChange();
    }

    for (int row = 0; row < imageSize.height(); row++) {
        auto line = m_image->scanLine(row);        
#ifdef Q_OS_WIN
        memcpy_s(line, imageSize.width(),
                 &imageData[row * imageSize.width()], imageSize.width());
#endif
#ifdef Q_OS_DARWIN
        memcpy(line, &imageData[row * imageSize.width()], imageSize.width());
#endif
    }
    //std::ofstream outFile("D:/image.raw", std::ios::out | std::ios::binary);
    //outFile.write((char*)pImageData, imageSize.width() * imageSize.height());
    //outFile.close();
    //m_image->save("D:/image.jpg");
    m_hasRealImage = true;
#ifdef TCP_SERVER
    QByteArray ba;
    QBuffer buf(&ba);
    m_image->save(&buf, "JPEG", 100);
    m_tcpServer->writeData(ba);
    //qDebug() << "LiveImageView::updateImage, " << QDateTime::currentMSecsSinceEpoch();
#else
    this->update();
#endif
}

void LiveImageView::clearImage() {
    QSize imageSize(800, 600);
    uchar* image = new uchar[imageSize.width() * imageSize.height()];
    memset(image, 0x7f, imageSize.width() * imageSize.height());
    updateImage(image, imageSize);
    delete []image;
    m_hasRealImage = false;
}

void LiveImageView::fit(const int destWidth, int destHeight)
{
    // 将原始尺寸图片放到容器中, 看左右两边留的空隙多还是上下两边留的空隙多,
    // 按照空隙少的那一边进行对齐(到容器边缘)
    double horzScale = static_cast<double>(destWidth) / static_cast<double>(m_image->width());
    double vertScale = static_cast<double>(destHeight) / static_cast<double>(m_image->height());
    double scale = 0;
    if (horzScale < vertScale) {
        scale = horzScale;
    } else {
        scale =  vertScale;
    }
    setZoom(scale * 100 - 0.1);
}

void LiveImageView::debugCutImage(const QRect rect)
{
    QImage img = m_image->copy(rect);
    QString fn = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/shst_cut.jpg";
    img.save(fn);
}


QRect LiveImageView::selectRect(const int &x1, const int &x2, const int &y1, const int &y2)
{
    int realX1 = qMin(x1, x2) - m_paintDstRect.left() + this->x();
    int realX2 = qMax(x1, x2) - m_paintDstRect.left() + this->x();
    int realY1 = qMin(y1, y2) - m_paintDstRect.top() + this->y();
    int realY2 = qMax(y1, y2) - m_paintDstRect.top() + this->y();

    if (realX1 < 0) realX1 = 0;
    if (realX2 < 0) realX2 = 0;
    if (realY1 < 0) realY1 = 0;
    if (realY2 < 0) realY2 = 0;

    if (m_zoom != 100) {
        realX1 = realX1 * 100 / m_zoom;
        realX2 = realX2 * 100 / m_zoom;
        realY1 = realY1 * 100 / m_zoom;
        realY2 = realY2 * 100 / m_zoom;
    }

    m_selectedRect.setLeft(realX1);
    m_selectedRect.setTop(realY1);
    m_selectedRect.setRight(realX2);
    m_selectedRect.setBottom(realY2);

    qDebug() << "LiveImageView, selectedRect:" << m_selectedRect;
    return m_selectedRect;
}

QRect LiveImageView::getSelectedRect() const
{
    return m_selectedRect;
}

bool LiveImageView::getZoomEnabled() const
{
    return m_zoomEnabled;
}

void LiveImageView::setZoomEnabled(bool zoomEnabled)
{
    if (m_zoomEnabled == zoomEnabled)
        return;
    m_zoomEnabled = zoomEnabled;
    showSizeChange();
    emit zoomEnabledChanged(zoomEnabled);
    if (zoomEnabled)
        emit zoomChanged(m_zoom);
}

int LiveImageView::getZoom()
{
    return m_zoom;
}

void LiveImageView::setZoom(int zoom)
{
    if (zoom > 10000)
        zoom = 10000;
    else if (zoom < 10)
        zoom = 10;

    if (zoom != m_zoom) {
        emit this->zoomChanging(zoom);
        m_zoom = zoom;
        showSizeChange();
        this->update();
        emit this->zoomChanged(zoom);
    }
}

int LiveImageView::getImageWidth() const
{
    return m_image->width();
}

int LiveImageView::getImageHeight() const
{
    return m_image->height();
}

int LiveImageView::getShowWidth() const
{
    return m_showWidth;
}

int LiveImageView::getShowHeight() const
{
    return m_showHeight;
}

void LiveImageView::showSizeChange()
{
    m_showWidth = m_image->width() * m_zoom / 100;
    m_showHeight = m_image->height() * m_zoom / 100;
    emit showWidthChanged(m_showWidth);
    emit showHeightChanged(m_showHeight);
}

#ifdef TCP_CLIENT
void LiveImageView::connectServer()
{
    m_tcpSocket->abort();
    m_tcpSocket->connectToHost(m_serverAddress, 60051);
}

void LiveImageView::disconnectServer()
{
    m_tcpSocket->disconnectFromHost();
}

void LiveImageView::readImageFromTcpServer()
{
    QByteArray imageBytes;

    while (m_previewEnabled) {
        m_imageStream.startTransaction();
        m_imageStream >> imageBytes;
        if (!m_imageStream.commitTransaction())
            return;

        m_image->loadFromData(imageBytes, "JPEG");
        this->update();
    }
}

void LiveImageView::displaySocketError(QAbstractSocket::SocketError socketError)
{

}

bool LiveImageView::isPreviewEnabled() const
{
    return m_previewEnabled;
}

void LiveImageView::setPreviewEnabled(bool previewEnabled)
{
    m_previewEnabled = previewEnabled;
    if (m_previewEnabled)
        connectServer();
    else
        disconnectServer();
}

QString LiveImageView::getServerAddress() const
{
    return m_serverAddress;
}

void LiveImageView::setServerAddress(const QString &serverAddress)
{
    m_serverAddress = serverAddress;
}
#endif
