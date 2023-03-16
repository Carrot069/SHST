#include "WzLiveImageView.h"

LiveImageView* g_liveImageView = nullptr;

LiveImageView::LiveImageView(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_image(new QImage(100, 100, QImage::Format_Indexed8)),
      m_colorTable()
{
    for(int i = 0; i < 256; ++i) m_colorTable.append(qRgb(i,i,i));
    m_image->setColorTable(m_colorTable);

    void* pImageData = m_image->bits();
    memset(pImageData, 0xCC, static_cast<size_t>(m_image->width() * m_image->height()));

    g_liveImageView = this;
}

LiveImageView::~LiveImageView()
{
    g_liveImageView = nullptr;
    delete m_image;
}

void LiveImageView::paint(QPainter *painter)
{
    int w = int(this->width());
    int h = int(this->height());
    QRect painterRect(0, 0, w, h);
    QRect imgRect(0, 0, m_image->width(), m_image->height());
    m_imageRect = WzUtils::fitRect(painterRect, imgRect, m_imageScale, true, false);

    painter->drawImage(m_imageRect, *m_image);
}

void LiveImageView::updateImage(const uchar *imageData, const int &imageBytes)
{
    bool ret = m_image->loadFromData(imageData, imageBytes, "JPG");
    this->update();
}

void LiveImageView::updateImage(const uchar* imageData, const QSize& imageSize)
{
    if (imageSize.width() != m_image->width() ||
            imageSize.height() != m_image->height()) {
        delete m_image;
        m_image = new QImage(imageSize, QImage::Format_Indexed8);
        m_image->setColorTable(m_colorTable);
    }

    for (int row = 0; row < imageSize.height(); row++) {
        auto line = m_image->scanLine(row);
        memcpy(line, &imageData[row * imageSize.width()], imageSize.width());
    }

    this->update();
}

void LiveImageView::clearImage() {
    QSize imageSize(800, 600);
    uchar* image = new uchar[imageSize.width() * imageSize.height()];
    memset(image, 0xCC, imageSize.width() * imageSize.height());
    updateImage(image, imageSize);
    delete []image;
}

void LiveImageView::selectRect(const int &x1, const int &x2, const int &y1, const int &y2)
{
    m_selectedRect.setLeft(qMax(0.0, (static_cast<double>(x1 - m_imageRect.x()) / m_imageScale) * 100.0));
    m_selectedRect.setTop(qMax(0.0, (static_cast<double>(y1 - m_imageRect.y()) / m_imageScale) * 100.0));
    m_selectedRect.setRight(qMin(static_cast<double>(m_image->width()), (static_cast<double>(x2 - m_imageRect.x()) / m_imageScale) * 100.0));
    m_selectedRect.setBottom(qMin(static_cast<double>(m_image->height()), (static_cast<double>(y2 - m_imageRect.y()) / m_imageScale) * 100.0));
    qDebug() << "LiveImageView, selectedRect:" << m_selectedRect;
}

QRect LiveImageView::getSelectedRect() const
{
    return m_selectedRect;
}
