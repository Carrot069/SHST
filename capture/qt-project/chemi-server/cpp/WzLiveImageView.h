#ifndef LIVEIMAGE_VIEW_H
#define LIVEIMAGE_VIEW_H

#ifdef ATIK
#include <comdef.h>
#endif

#include <QtQuick>
#include <QSize>
#ifdef TCP_SERVER
#include "WzTcpServer.h"
#endif
#ifdef TCP_CLIENT
#include <QDataStream>
#include <QTcpSocket>
#endif

class LiveImageView : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit LiveImageView(QQuickItem *parent = nullptr);
    ~LiveImageView();

    void paint(QPainter *painter);

    Q_INVOKABLE void updateImage(const uchar* imageData, const QSize& imageSize);
    Q_INVOKABLE void clearImage();

signals:

public slots:

private:
    QImage* m_image;
    QVector<QRgb> m_colorTable;
};

extern LiveImageView* g_liveImageView;

#endif // LIVEIMAGE_VIEW_H
