#ifndef LIVEIMAGE_VIEW_H
#define LIVEIMAGE_VIEW_H

#include <QtQuick>
#include <QSize>
#include "WzUtils.h"

class LiveImageView : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit LiveImageView(QQuickItem *parent = nullptr);
    ~LiveImageView();

    void paint(QPainter *painter);

    Q_INVOKABLE void updateImage(const uchar* imageData, const QSize& imageSize, const int imageBytes, const QImage::Format format);
    Q_INVOKABLE void updateImage(const uchar* imageData, const QSize& imageSize);
    Q_INVOKABLE void clearImage();
    // QML MouseArea 划出的方形区域, 这个区域没有经过缩放, 所以不能直接使用, 传入这个函数后会根据图片缩放比例重新调整
    Q_INVOKABLE void selectRect(const int &x1, const int &x2, const int &y1, const int &y2);

    // 经过缩放的选择的矩形区域
    QRect getSelectedRect() const;

    bool flipHoriz() const;
    void setFlipHoriz(bool newFlipHoriz);

signals:

public slots:

private:
    double m_imageScale = 100;
    QRect m_imageRect;
    QRect m_selectedRect;
    QImage* m_image;
    QVector<QRgb> m_colorTable;
    bool m_flipHoriz = false;
};

extern LiveImageView* g_liveImageView;

#endif // LIVEIMAGE_VIEW_H
