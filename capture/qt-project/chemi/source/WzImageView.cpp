#include "WzImageView.h"

WzImageView* g_imageView = nullptr;

WzImageView::WzImageView(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_image(new QImage(100, 100, QImage::Format_ARGB32)),
      m_colorTable()
{
    for(int i = 0; i < 256; ++i)
        m_colorTable.insert(0, qRgb(i,i,i));

    m_colorTableOriginal = m_colorTable;

    void* pImageData = m_image->bits();
    memset(pImageData, 0xCC, static_cast<size_t>(m_image->width() * m_image->height()));

    setAcceptedMouseButtons(Qt::MiddleButton);

    g_imageView = this;
}

WzImageView::~WzImageView()
{    
    g_imageView = nullptr;
    delete m_image;

    if (nullptr != m_paintImage) {
        delete m_paintImage;
        m_paintImage = nullptr;
    }
}

void WzImageView::paint(QPainter *painter)
{
    QPainter painter3;
    QPainter* painterBak = painter;
    painter = &painter3;

//    if (nullptr != m_paintImage) {
//        delete m_paintImage;
//        m_paintImage = nullptr;
//    }
//    m_paintImage = new QImage(*m_image);

    painter->begin(m_paintImage);

    painter->fillRect(QRect(0, 0,
                            static_cast<int>(m_paintImage->width()),
                            static_cast<int>(m_paintImage->height())),
                      QBrush(QColor::fromRgb(127, 127, 127)));

    painter->drawImage(m_paintImage->rect(), *m_image);

    if (!m_isRGBImage && m_showColorBar) {
        QImage colorBar(1, 256, QImage::Format_Indexed8);
        colorBar.setColorTable(m_colorTable);
        uchar* bits = colorBar.bits();
        for (int i = 0; i < 256; i++) {
            *bits = i;
            bits++;
            *bits = i;
            bits++;
            *bits = i;
            bits++;
            *bits = 0;
            bits++;
        }

        int colorBarHeight = this->height() * 0.5;
        int colorBarWidth = colorBarHeight * 0.078;
        QRect colorBarRect(m_paintImage->width() - colorBarWidth,
                           m_paintImage->height() - colorBarHeight,
                           colorBarWidth,
                           colorBarHeight);
        painter->drawImage(colorBarRect, colorBar);

        painter->setCompositionMode(QPainter::CompositionMode_Exclusion);
        QPen pen(qRgb(255, 255, 255));
        QFont font("Arial", 12);
        QFontMetrics fm(font);
        painter->setFont(font);
        painter->setPen(pen);

        QString grayHighStr = QString::number(m_grayHigh);
        QString grayLowStr = QString::number(m_grayLow);
        QString grayMiddleStr = QString::number((m_grayHigh - m_grayLow) / 2 + m_grayLow);

        painter->drawText(m_paintImage->width() - colorBarWidth - 5 - fm.width(grayHighStr),
                          m_paintImage->height() - colorBarHeight + fm.height() - 4,
                          grayHighStr);
        painter->drawText(m_paintImage->width() - colorBarWidth - 5 - fm.width(grayLowStr),
                          m_paintImage->height() - 4,
                          grayLowStr);
        painter->drawText(m_paintImage->width() - colorBarWidth - 5 - fm.width(grayMiddleStr),
                          m_paintImage->height() - colorBarHeight / 2,
                          grayMiddleStr);
    }

    painter->end();

    painter = painterBak;
    int w = int(this->width());
    int h = int(this->height());
    QRect rectPaint(0,0,w,h);
    QImage scaledImage = m_paintImage->scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    painter->drawImage(rectPaint, scaledImage);
}

void WzImageView::updateImage(uint8_t *imageData, const int& imageWidth, const int& imageHeight)
{
    m_isRGBImage = false;

    if (nullptr != m_image) {
        delete m_image;
        m_image = nullptr;
    }

    m_image = new QImage(imageWidth, imageHeight, QImage::Format_ARGB32);
    qDebug() << "m_image->sizeInBytes:" << m_image->sizeInBytes();
    this->setWidth(imageWidth * m_zoom / 100);
    this->setHeight(imageHeight * m_zoom / 100);

    if (nullptr != m_paintImage) {
        delete m_paintImage;
        m_paintImage = nullptr;
    }
    m_paintImage = new QImage(m_image->width(), m_image->height(), QImage::Format_ARGB32);
    m_paintImage->fill(Qt::gray);

    uint8_t* pImageData = imageData;
    for (int row = 0; row < imageHeight; row++) {
        uchar* line = m_image->scanLine(row);
        for (int col = 0; col < imageWidth; col++) {
            *line = *pImageData;
            line++;
            pImageData++;
        }
    }
    //void* pImageData = m_image->bits();
    //memcpy(pImageData, imageData, static_cast<size_t>(imageWidth * imageHeight));
    this->update();
}

void WzImageView::updateImage(const QByteArray & imageData, const QSize& imageSize)
{
    if (imageSize.width() != m_image->width() ||
            imageSize.height() != m_image->height()) {
        delete m_image;
        m_image = new QImage(imageSize, QImage::Format_ARGB32);
        this->setWidth(imageSize.width() * m_zoom / 100);
        this->setHeight(imageSize.height() * m_zoom / 100);

        if (nullptr != m_paintImage) {
            delete m_paintImage;
            m_paintImage = nullptr;
        }
        m_paintImage = new QImage(m_image->width(), m_image->height(), QImage::Format_ARGB32);
        m_paintImage->fill(Qt::gray);
    }

    void* pImageData = m_image->bits();
    memcpy(pImageData, imageData.constData(), static_cast<size_t>(imageData.length()));
    this->update();
}

void WzImageView::updateImage(uint32_t *imageData, const int& imageWidth, const int& imageHeight)
{
    m_isRGBImage = true;

    if (nullptr != m_image) {
        delete m_image;
        m_image = nullptr;
    }

    m_image = new QImage(imageWidth, imageHeight, QImage::Format_ARGB32);
    qDebug() << "m_image->sizeInBytes:" << m_image->sizeInBytes();
    this->setWidth(imageWidth * m_zoom / 100);
    this->setHeight(imageHeight * m_zoom / 100);

    if (nullptr != m_paintImage) {
        delete m_paintImage;
        m_paintImage = nullptr;
    }
    m_paintImage = new QImage(m_image->width(), m_image->height(), QImage::Format_ARGB32);
    m_paintImage->fill(Qt::gray);

    memcpy(m_image->bits(), imageData, imageWidth * imageHeight * sizeof(uint32_t));

    this->update();
}



void WzImageView::fit(int destWidth, int destHeight) {
    if (nullptr == m_image)
        return;
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

QImage* WzImageView::image() {
    return m_image;
}

QImage* WzImageView::paintImage() {
    return m_paintImage;
}

void WzImageView::wheelEvent(QWheelEvent *event) {
    if (m_image == nullptr)
        return;
    int zoom = m_zoom;
    if (event->delta() > 0) {
        if (zoom < 500)
            zoom = zoom + 10;
    } else {
        if (zoom > 10)
            zoom = zoom - 10;
    }
    setZoom(zoom);
}

QVariant WzImageView::getColorTable() {
    QList<QVariant> result;
    for (int n = 0; n < m_colorTable.count(); n++) {
        QVariantMap map;
        map["R"] = qRed(m_colorTable[n]);
        map["G"] = qGreen(m_colorTable[n]);
        map["B"] = qBlue(m_colorTable[n]);
        result.append(map);
    }
    return QVariant(result);
}

void WzImageView::setColorTable(QVariant colorTable) {
    QList<QVariant> list = colorTable.toList();
    m_colorTable.clear();
    m_colorTableOriginal.clear();
    for (int n = 0; n < list.count(); n++) {
        QMap<QString, QVariant> map = list[n].toMap();
        m_colorTableOriginal.append(qRgb(map["R"].toInt(), map["G"].toInt(), map["B"].toInt()));
    }
    if (m_colorTableInvert) {
        for (int n = 0; n < m_colorTableOriginal.count(); n++) {
            m_colorTable.insert(0, m_colorTableOriginal[n]);
        }
    } else {
        m_colorTable = m_colorTableOriginal;
    }
    emit colorTableChanged();
    update();
}

bool WzImageView::getShowColorBar() {
    return m_showColorBar;
};

void WzImageView::setShowColorBar(bool showColorBar) {
    if (m_showColorBar == showColorBar)
        return;
    m_showColorBar = showColorBar;
    emit showColorBarChanged(showColorBar);
    update();
};

bool WzImageView::getColorTableInvert() {
    return m_colorTableInvert;
};

void WzImageView::setColorTableInvert(bool invert) {
    if (m_colorTableInvert == invert)
        return;
    m_colorTableInvert = invert;
    emit colorTableInvertChanged();
    m_colorTable.clear();
    if (m_colorTableInvert) {
        for (int n = 0; n < m_colorTableOriginal.count(); n++) {
            m_colorTable.insert(0, m_colorTableOriginal[n]);
        }
    } else {
        m_colorTable = m_colorTableOriginal;
    }
};

int WzImageView::getGrayLow() {
    return m_grayLow;
};

void WzImageView::setGrayLow(int grayLow) {
    if (m_grayLow == grayLow)
        return;
    m_grayLow = grayLow;
    emit grayLowChanged();
};

int WzImageView::getGrayHigh() {
    return m_grayHigh;
};

void WzImageView::setGrayHigh(int grayHigh) {
    if (m_grayHigh == grayHigh)
        return;
    m_grayHigh = grayHigh;
    emit grayHighChanged();
};

int WzImageView::getZoom() {
    return m_zoom;
}

void WzImageView::setZoom(int zoom) {
    if (zoom > 500)
        zoom = 500;
    else if (zoom < 10)
        zoom = 10;

    if (zoom != m_zoom) {
        emit this->zoomChanging(zoom);
        m_zoom = zoom;
        this->setWidth(m_image->width() * zoom / 100);
        this->setHeight(m_image->height() * zoom / 100);
        this->update();
        emit this->zoomChanged(zoom);
    }
}
