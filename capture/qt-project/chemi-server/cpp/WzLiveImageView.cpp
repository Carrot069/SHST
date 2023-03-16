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
    QRect rect(0,0,w,h);

    QPoint p;
    p.setX((this->width() - m_image->width()) / 2);
    p.setY((this->height() - m_image->height()) / 2);

    if (p.x() < 0 || p.y() < 0)
        painter->drawImage(rect, *m_image);
    else
        painter->drawImage(p, *m_image);
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
#ifdef Q_OS_MAC
        memcpy(line, &imageData[row * imageSize.width()], imageSize.width());
#else
        memcpy_s(line, imageSize.width(),
                 &imageData[row * imageSize.width()], imageSize.width());
#endif
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
