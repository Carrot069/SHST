#include "WzImageView.h"

WzImageView* g_imageView = nullptr;

WzImageView::WzImageView(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_image(new QImage(100, 100, QImage::Format_Indexed8)),
      m_colorTable()
{
    m_showWidth = m_image->width() * m_zoom / 100;
    m_showHeight = m_image->height() * m_zoom / 100;

    for(int i = 0; i < 256; ++i) m_colorTable.insert(0, qRgb(i,i,i));
    m_colorTableOriginal = m_colorTable;
    m_image->setColorTable(m_colorTable);

    void* pImageData = m_image->bits();
    memset(pImageData, 0x7f, static_cast<size_t>(m_image->width() * m_image->height()));

    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setKeepMouseGrab(true);
    setFlag(ItemAcceptsInputMethod, true);
    setFlag(ItemIsFocusScope);

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
    QRect dstRect(0, 0, 0, 0);
    QRect srcRect(0, 0, m_image->width(), m_image->height());

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

    m_imageSrcRect = srcRect;
    m_paintDstRect = dstRect;
    QPoint offsetPoint(this->x(), this->y());
    //qInfo() << "srcRect:" << srcRect;

    QPainter painterLocal;
    QPainter* painterBak = painter;
    painter = &painterLocal;

    QImage img(srcRect.width(), srcRect.height(), QImage::Format_ARGB32);
    img.fill(Qt::gray);
    painter->begin(&img);
    painter->drawImage(QPoint(0, 0), *m_image, srcRect);
    painter->end();

    if (nullptr != m_paintImage) {
        delete m_paintImage;
        m_paintImage = nullptr;
    }
    m_paintImage = new QImage(img.scaled(dstRect.width(), dstRect.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    painter->begin(m_paintImage);

    if (m_showColorBar) {
        drawColorBar(painter);
    }

    if (m_action == WzEnum::CropImage && (m_isMousePressed || m_isCropImageSelected)) {
        QPen pen(Qt::DotLine);
        pen.setColor(Qt::red);
        pen.setWidth(2);
        painter->setPen(pen);
        QRect r = WzUtils::absRect(QRect(m_mousePressPoint - offsetPoint, m_mouseMovePos - offsetPoint));
        painter->drawRect(r);
        QRect r2 = WzUtils::absRect(QRect(m_mousePressPoint - offsetPoint + m_paintDstRect.topLeft(),
                                          m_mouseMovePos - offsetPoint + m_paintDstRect.topLeft()));
        emit cropImageRectMoving(r2);
    }

    if (m_action == WzEnum::HorizontalRotate && m_isMousePressed) {
        QPen pen(Qt::DotLine);
        pen.setColor(Qt::red);
        painter->setPen(pen);
        painter->drawLine(m_mousePressPoint - offsetPoint, m_mouseMovePos - offsetPoint);
        m_imageRotateAngle = WzUtils::getAngle(m_mousePressPoint, m_mouseMovePos);
        qDebug() << "angle:" << m_imageRotateAngle;
    }

    painter->end();

    painter = painterBak;
    painter->drawImage(dstRect, *m_paintImage);
}

void WzImageView::updateImage(uint8_t *imageData, const int& imageWidth, const int& imageHeight,
                              WzImageBuffer *imageBuffer)
{
    m_imageBuffer = imageBuffer;

    if (nullptr != m_image) {
        if (m_image->width() != imageWidth || m_image->height() != imageHeight) {
            delete m_image;
            m_image = nullptr;
        }
    }

    if (!m_image) {
        m_image = new QImage(imageWidth, imageHeight, QImage::Format_Indexed8);
        showSizeChange();
    }
    m_image->setColorTable(m_colorTable);
    qDebug() << "m_image->sizeInBytes:" << m_image->sizeInBytes();

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

void WzImageView::updateImage(const QByteArray & imageData, const QSize& imageSize,
                              WzImageBuffer *imageBuffer)
{
    m_imageBuffer = imageBuffer;
    if (imageSize.width() != m_image->width() ||
            imageSize.height() != m_image->height()) {
        delete m_image;
        m_image = new QImage(imageSize, QImage::Format_Indexed8);
        m_image->setColorTable(m_colorTable);
        showSizeChange();

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

void WzImageView::updateImage(uint32_t *imageData, const int& imageWidth, const int& imageHeight,
                              WzImageBuffer *imageBuffer)
{
    m_imageBuffer = imageBuffer;
    m_isRGBImage = true;

    if (nullptr != m_image) {
        delete m_image;
        m_image = nullptr;
    }

    m_image = new QImage(imageWidth, imageHeight, QImage::Format_ARGB32);
    qDebug() << "m_image->sizeInBytes:" << m_image->sizeInBytes();
    showSizeChange();

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

void WzImageView::cropImageApply()
{
    m_isCropImageSelected = false;
    setAction(WzEnum::AnalysisActionNone);
    update(this->boundingRect().toRect());
}

void WzImageView::cropImageCancel()
{
    m_isCropImageSelected = false;
    m_mousePressPoint = m_mouseMovePos;
    setAction(WzEnum::AnalysisActionNone);
    emit cropImageUnselect();
    update(this->boundingRect().toRect());
}

void WzImageView::cropImageReselect()
{
    m_isCropImageSelected = false;
    m_mousePressPoint = m_mouseMovePos;
    emit cropImageUnselect();
    update(this->boundingRect().toRect());
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
        if (zoom < 10000)
            zoom = zoom * 1.1;
    } else {
        if (zoom > 10)
            zoom = zoom * 0.9;
    }
    setZoom(zoom);
}

void WzImageView::hoverEnterEvent(QHoverEvent *event)
{
    if (m_action == WzEnum::CropImage) {
        event->accept();
        return;
    }
}

void WzImageView::hoverMoveEvent(QHoverEvent *event)
{
    if (!m_imageBuffer)
        return;
    QPoint offsetPoint = QPoint(this->x(), this->y()) - m_paintDstRect.topLeft();
    QHoverEvent eventOffset(event->type(), event->pos() + offsetPoint, event->oldPos() + offsetPoint, event->modifiers());
    QPoint realPos = WzUtils::noZoomPoint(eventOffset.pos(), m_zoom);
    int currentGray = 0;
    if (realPos.x() >= 0 && static_cast<uint32_t>(realPos.x()) < m_imageBuffer->width &&
        realPos.y() >= 0 && static_cast<uint32_t>(realPos.y()) < m_imageBuffer->height) {
        m_imageBuffer->getPixel(realPos.y(), realPos.x(), currentGray);
    }
    emit currentGrayChanged(realPos.x(), realPos.y(), currentGray);

    switch(m_action) {
    case WzEnum::DeleteAnnotation:
    case WzEnum::RemoveBand:
    case WzEnum::RemoveLane:
        return;
    default:
        break;
    }

    if (m_action == WzEnum::CropImage) {
        setCursor(Qt::CrossCursor);
        event->accept();
        return;
    }
    if (m_action == WzEnum::HorizontalRotate) {
        setCursor(Qt::CrossCursor);
        event->accept();
        return;
    }
    if (m_action == WzEnum::SelectBackground) {
        setCursor(Qt::CrossCursor);
        event->accept();
        return;
    }
    eventOffset.ignore();
    /*
    foreach(WzPaintedItem *item, findChildren<WzPaintedItem*>()) {
        item->hoverMoveEvent(&eventOffset);
        if (eventOffset.isAccepted()) {
            event->accept();
            return;
        }
    }
    if (m_action == WzEnum::AddRectangle ||
        m_action == WzEnum::AddEllipse ||
        m_action == WzEnum::AddArrow ||
        m_action == WzEnum::AddText) {
        setCursor(Qt::CrossCursor);
        event->accept();
        return;
    }

    m_areas->hoverMoveEvent(this, &eventOffset);
    */
    event->accept();
}

void WzImageView::hoverLeaveEvent(QHoverEvent *event)
{
    if (m_action == WzEnum::CropImage) {
        event->accept();
        return;
    }
    //m_areas->hoverLeaveEvent(this, event);
}

void WzImageView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        emit popupMenu(event->pos().x(), event->pos().y());
        return;
    }
    forceActiveFocus();
    m_isMousePressed = true;

    QPoint offsetPoint = QPoint(this->x(), this->y()) - m_paintDstRect.topLeft();
    event->setLocalPos(event->localPos() + offsetPoint);

    m_mousePressPoint = event->pos();

    if (m_action == WzEnum::CropImage) {
        m_mouseMovePos = event->pos();
        m_mousePressPosNoZoom = WzUtils::noZoomPoint(m_mousePressPoint, m_zoom);
        m_mouseMovePosNoZoom = WzUtils::noZoomPoint(m_mouseMovePos, m_zoom);
        emit cropImageUnselect();
        update(this->boundingRect().toRect());
        event->accept();
        return;
    }

    if (m_action == WzEnum::HorizontalRotate) {
        m_imageRotateAngle = 0;
        event->accept();
        return;
    }

    /*
    if (m_action == WzEnum::SelectBackground) {
        m_drawSelectBackgroundRect = true;
        event->accept();
        return;
    }

    m_activePaintedItem = nullptr;
    event->ignore();
    foreach(WzPaintedItem *item, findChildren<WzPaintedItem*>()) {
        item->setImageOffset(QPoint(this->x(), this->y()));
        item->setPainterOffset(m_paintDstRect.topLeft());
        // 目前为止还没有条目接受事件, 所以继续调用条目的事件处理函数
        if (!event->isAccepted())
            item->mousePressEvent(event);
        // 事件已经被接受, 将其他条目设置为非焦点状态
        else if (m_activePaintedItem)
            item->setFocus(false);
        // 调用过条目的事件处理函数后, 如果被接受了则进行对应处理
        if (event->isAccepted()) {
            if (m_action == WzEnum::DeleteAnnotation) {
                QRect updateRect = item->getPaintRect();
                delete item;
                update(updateRect);
                m_activePaintedItem = nullptr;
            } else {
                m_activePaintedItem = item;
            }
        } else {
            item->setFocus(false);
        }
    }

    if (addAnnotationRect(event)) return;
    if (addAnnotationArrow(event)) return;
    if (addAnnotationText(event)) return;

    m_areas->mousePressEvent(this, event);
    if (!event->isAccepted()) {
        m_areas->setActive(false);
    }
    */

    event->accept();
}

void WzImageView::mouseMoveEvent(QMouseEvent *event)
{
    QPoint offsetPoint = QPoint(this->x(), this->y()) - m_paintDstRect.topLeft();
    event->setLocalPos(event->localPos() + offsetPoint);

    if (m_action == WzEnum::CropImage) {
        m_isCropImageSelected = false;
        QRect updateRect = WzUtils::absRect(m_mousePressPoint - offsetPoint, m_mouseMovePos - offsetPoint);
        updateRect.adjust(0, 0, 2, 2);
        update(updateRect);
        m_mouseMovePos = event->pos();
        m_mouseMovePosNoZoom = WzUtils::noZoomPoint(m_mouseMovePos, m_zoom);
        updateRect = WzUtils::absRect(m_mousePressPoint - offsetPoint, m_mouseMovePos - offsetPoint);
        updateRect.adjust(0, 0, 2, 2);
        update(updateRect);
        event->accept();
        return;
    }

    if (m_action == WzEnum::HorizontalRotate) {
        QRect updateRect = WzUtils::absRect(m_mousePressPoint - offsetPoint, m_mouseMovePos - offsetPoint);
        updateRect.adjust(0, 0, 1, 1);
        update(updateRect);
        m_mouseMovePos = event->pos();
        updateRect = WzUtils::absRect(m_mousePressPoint - offsetPoint, m_mouseMovePos - offsetPoint);
        updateRect.adjust(0, 0, 1, 1);
        update(updateRect);
        event->accept();
        return;
    }
    if (m_action == WzEnum::SelectBackground) {
        QRect updateRect = WzUtils::absRect(m_mousePressPoint - offsetPoint, m_mouseMovePos - offsetPoint);
        updateRect.adjust(0, 0, 1, 1);
        update(updateRect);
        m_mouseMovePos = event->pos();
        updateRect = WzUtils::absRect(m_mousePressPoint - offsetPoint, m_mouseMovePos - offsetPoint);
        updateRect.adjust(0, 0, 1, 1);
        update(updateRect);
        event->accept();
        return;
    }

    event->ignore();
    /*
    foreach(WzPaintedItem *item, findChildren<WzPaintedItem*>()) {
        item->mouseMoveEvent(event);
        if (event->isAccepted())
            return;
    }

    m_areas->mouseMoveEvent(this, event);
    */

    event->accept();
}

void WzImageView::mouseReleaseEvent(QMouseEvent *event)
{
    QPoint p2 = -QPoint(this->x(), this->y()) + m_paintDstRect.topLeft();
    QPoint offsetPoint = QPoint(this->x(), this->y()) - m_paintDstRect.topLeft();
    event->setLocalPos(event->localPos() + offsetPoint);

    m_isMousePressed = false;
    if (m_action == WzEnum::CropImage && event->button() == Qt::LeftButton) {
        if (m_mousePressPoint != m_mouseMovePos) {
            m_isCropImageSelected = true;
            emit cropImageSelected(WzUtils::absRect(m_mousePressPoint + p2, m_mouseMovePos + p2));
        }
        return;
    }
    if (m_action == WzEnum::HorizontalRotate && m_imageRotateAngle != 0 &&
                event->button() == Qt::LeftButton) {
        emit imageRotate(m_imageRotateAngle);
        this->update();
        setAction(WzEnum::AnalysisActionNone);
    }
    /*
    if (m_action == WzEnum::SelectBackground) {
        m_action = WzEnum::AnalysisActionNone;
        m_drawSelectBackgroundRect = false;
        QRect updateRect(WzUtils::absRect(m_mousePressPoint - offsetPoint, m_mouseMovePos - offsetPoint));
        updateRect.adjust(0, 0, 1, 1);
        update(updateRect);
        m_areas->setSubtractBackground(true);
        m_areas->setSubtractBackgroundValue(calcBackgroundValue(WzUtils::absRect(m_mousePressPoint, m_mouseMovePos)));
        areasAnalysisDataChanged();
        return;
    }
    switch(m_action) {
    case WzEnum::AddArrow:
    case WzEnum::AddEllipse:
    case WzEnum::AddRectangle:
    case WzEnum::AddText:
        if (!event->modifiers().testFlag(Qt::ShiftModifier))
            setAction(WzEnum::AnalysisActionNone);
        break;
    default:
        break;
    }

    event->ignore();
    foreach(WzPaintedItem *item, findChildren<WzPaintedItem*>()) {
        item->mouseReleaseEvent(event);
        if (event->isAccepted())
            return;
    }
    m_areas->mouseReleaseEvent(this, event);
    */

    event->accept();
}

void WzImageView::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->ignore();
    /*
    foreach(WzPaintedItem *item, findChildren<WzPaintedItem*>()) {
        item->mouseDoubleClickEvent(event);
        if (event->isAccepted())
            return;
    }
    */
    event->accept();
}

void WzImageView::keyReleaseEvent(QKeyEvent *event) {
    //m_areas->keyReleaseEvent(this, event);
}

WzEnum::AnalysisAction WzImageView::getAction() const
{
    return m_action;
}

void WzImageView::setAction(const WzEnum::AnalysisAction &action)
{
    if (m_action == action)
        return;
    m_action = action;
    switch (m_action) {
    case WzEnum::AnalysisActionNone:
        unsetCursor();
        break;
    case WzEnum::HorizontalRotate:
    case WzEnum::CropImage:
    case WzEnum::AddRectangle:
    case WzEnum::AddEllipse:
    case WzEnum::AddArrow:
    case WzEnum::AddText: {
        setCursor(Qt::CrossCursor);
        break;
    }
    case WzEnum::DeleteAnnotation:
    case WzEnum::RemoveBand:
    case WzEnum::RemoveLane:
        setCursor(QCursor(QPixmap(":/images/delete-16x16-red.png")));
        break;
    default:
        break;
    }
    emit actionChanged(action);
    //m_areas->setAction(action);
}

int WzImageView::getImageWidth() const
{
    if (m_image)
        return m_image->width();
    else
        return 0;
}

int WzImageView::getImageHeight() const
{
    if (m_image)
        return m_image->height();
    else
        return 0;
}

QRect WzImageView::getSelectedRect()
{
    return WzUtils::absRect(m_mousePressPoint, m_mouseMovePos);
}

QRect WzImageView::getSelectedRectNoZoom()
{
    return WzUtils::absRect(m_mousePressPosNoZoom, m_mouseMovePosNoZoom);
}

int WzImageView::getShowWidth() const
{
    return m_showWidth;
}

int WzImageView::getShowHeight() const
{
    return m_showHeight;
}
void WzImageView::showSizeChange()
{
    m_showWidth = m_image->width() * m_zoom / 100;
    m_showHeight = m_image->height() * m_zoom / 100;
    emit showWidthChanged(m_showWidth);
    emit showHeightChanged(m_showHeight);
}

void WzImageView::drawColorBar(QPainter *painter)
{
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
    m_image->setColorTable(m_colorTable);
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
    m_colorTable.clear();
    if (m_colorTableInvert) {
        for (int n = 0; n < m_colorTableOriginal.count(); n++) {
            m_colorTable.insert(0, m_colorTableOriginal[n]);
        }
    } else {
        m_colorTable = m_colorTableOriginal;
    }
    m_image->setColorTable(m_colorTable);
};

int WzImageView::getGrayLow() {
    return m_grayLow;
};

void WzImageView::setGrayLow(int grayLow) {
    if (m_grayLow == grayLow)
        return;
    m_grayLow = grayLow;
};

int WzImageView::getGrayHigh() {
    return m_grayHigh;
};

void WzImageView::setGrayHigh(int grayHigh) {
    if (m_grayHigh == grayHigh)
        return;
    m_grayHigh = grayHigh;
};

int WzImageView::getZoom() {
    return m_zoom;
}

void WzImageView::setZoom(int zoom) {
    if (zoom > 10000)
        zoom = 10000;
    else if (zoom < 10)
        zoom = 10;

    if (zoom != m_zoom) {
        emit this->zoomChanging(zoom);
        m_zoom = zoom;
        m_mousePressPoint = WzUtils::zoomPoint(m_mousePressPosNoZoom, m_zoom);
        m_mouseMovePos = WzUtils::zoomPoint(m_mouseMovePosNoZoom, m_zoom);
        showSizeChange();
        this->update();
        emit this->zoomChanged(zoom);
    }
}
