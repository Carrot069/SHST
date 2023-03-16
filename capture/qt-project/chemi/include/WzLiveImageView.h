#ifndef LIVEIMAGE_VIEW_H
#define LIVEIMAGE_VIEW_H

#include <QtQuick>
#include <QSize>
#ifdef TCP_SERVER
#include "WzTcpServer.h"
#endif
#ifdef TCP_CLIENT
#include <QDataStream>
#include <QTcpSocket>
#endif

#include "WzUtils.h"

class LiveImageView : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit LiveImageView(QQuickItem *parent = nullptr);
    ~LiveImageView();

    void paint(QPainter *painter);

    Q_INVOKABLE void updateImage(const uchar* imageData, const QSize& imageSize);
    Q_INVOKABLE void clearImage();
    Q_INVOKABLE void fit(const int destWidth, const int destHeight);
    // 调试用, 根据传入的矩形截取实时预览画面并保存到桌面
    Q_INVOKABLE void debugCutImage(const QRect rect);
    // QML MouseArea 划出的方形区域, 这个区域没有经过缩放, 所以不能直接使用, 传入这个函数后会根据图片缩放比例重新调整
    Q_INVOKABLE QRect selectRect(const int &x1, const int &x2, const int &y1, const int &y2);
    Q_PROPERTY(int zoom READ getZoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(int imageWidth READ getImageWidth NOTIFY imageWidthChanged)
    Q_PROPERTY(int imageHeight READ getImageHeight NOTIFY imageHeightChanged)
    Q_PROPERTY(int showWidth READ getShowWidth NOTIFY showWidthChanged)
    Q_PROPERTY(int showHeight READ getShowHeight NOTIFY showHeightChanged)
    Q_PROPERTY(bool zoomEnabled READ getZoomEnabled WRITE setZoomEnabled NOTIFY zoomEnabledChanged)

    // 经过缩放的选择的矩形区域
    QRect getSelectedRect() const;
#ifdef TCP_CLIENT
    Q_PROPERTY(bool previewEnabled READ isPreviewEnabled WRITE setPreviewEnabled)
    Q_PROPERTY(QString serverAddress READ getServerAddress WRITE setServerAddress)
#endif

signals:
    void zoomEnabledChanged(bool enabled);
    void zoomChanging(int zoom);
    void zoomChanged(int zoom);
    void imageWidthChanged(int imageWidth);
    void imageHeightChanged(int imageHeight);
    void showWidthChanged(int showWidth);
    void showHeightChanged(int showHeight);

public slots:

private:
    // 是否有真实图像可供显示
    bool m_hasRealImage = false;
    QImage* m_image;
    QVector<QRgb> m_colorTable;

#ifdef TCP_SERVER
    WzTcpServer* m_tcpServer = nullptr;
#endif
#ifdef TCP_CLIENT
    void connectServer();
    void disconnectServer();
    void readImageFromTcpServer();
    void displaySocketError(QAbstractSocket::SocketError socketError);

    bool isPreviewEnabled() const;
    void setPreviewEnabled(bool previewEnabled);

    QString getServerAddress() const;
    void setServerAddress(const QString &getServerAddress);

    QTcpSocket* m_tcpSocket = nullptr;
    QDataStream m_imageStream;
    // 暂时的属性
    bool m_previewEnabled = false;
    QString m_serverAddress;
#endif
    QRect m_imageSrcRect;
    QRect m_paintDstRect;
    QRect m_selectedRect;
    bool m_zoomEnabled = false;
    int m_zoom = 100;
    int m_showWidth = 1;
    int m_showHeight = 1;
    int getZoom();
    void setZoom(int zoom);
    int getImageWidth() const;
    int getImageHeight() const;
    int getShowWidth() const;
    int getShowHeight() const;
    bool getZoomEnabled() const;
    void setZoomEnabled(bool zoomEnabled);

    void showSizeChange();
};

extern LiveImageView* g_liveImageView;

#endif // LIVEIMAGE_VIEW_H
