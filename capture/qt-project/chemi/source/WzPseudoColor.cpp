#include "WzPseudoColor.h"

WzPseudoColor::WzPseudoColor(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_image(new QImage(256, 1, QImage::Format_RGB888))
{
    QList<QVariant> list;
    for (int n = 0; n < 256; n++) {
        QMap<QString, QVariant> map;
        map["R"] = 255-n;
        map["G"] = 255-n;
        map["B"] = 255-n;
        list.append(map);
    }
    m_rgbList = list;
    updateImage();
}

WzPseudoColor::~WzPseudoColor() {
    delete m_image;
}

void WzPseudoColor::paint(QPainter *painter) {
    QRect rect(0, 0, this->width(), this->height());
    painter->drawImage(rect, *m_image);
}

void WzPseudoColor::updateImage() {
    QList<QVariant> list = m_rgbList.toList();
    uchar* pImageBits = m_image->bits();
    int count = list.count();
    if (count > 256) count = 256;
    for (int n = 0; n < count; n++) {
        QMap<QString, QVariant> map = list[m_vertical ? count - 1 - n : n].toMap();
        *pImageBits = map["R"].toInt() & 0xff;
        pImageBits++;
        *pImageBits = map["G"].toInt() & 0xff;
        pImageBits++;
        *pImageBits = map["B"].toInt() & 0xff;
        pImageBits++;
        if (m_image->bytesPerLine() == 4)
            pImageBits++;
    }
    update();
}

QVariant WzPseudoColor::getRgbList() {
    return m_rgbList;
}

void WzPseudoColor::setRgbList(QVariant rgbList) {
    m_rgbList = rgbList;
    emit rgbListChanged();
    updateImage();
}

bool WzPseudoColor::getVertical() {
    return m_vertical;
}

void WzPseudoColor::setVertical(bool vertical) {
    if (m_vertical == vertical)
        return;
    m_vertical = vertical;
    if (nullptr != m_image) {
        delete m_image;
        m_image = nullptr;
    }
    if (m_vertical) {
        m_image = new QImage(1, 256, QImage::Format_RGB888);
    } else {
        m_image = new QImage(256, 1, QImage::Format_RGB888);
    }
    updateImage();
    emit verticalChanged(vertical);
}
