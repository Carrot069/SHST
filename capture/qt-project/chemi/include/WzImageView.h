#ifndef WZIMAGE_VIEW_H
#define WZIMAGE_VIEW_H

#include <QtQuick>
#include <QSize>
#include <QFont>
#include <QFontMetrics>

class WzImageView : public QQuickPaintedItem
{
    Q_OBJECT

public:
    static void drawColorBar(QImage& image, int grayLow, int grayHigh, QVariant rgbList);

    explicit WzImageView(QQuickItem *parent = nullptr);
    ~WzImageView();

    void paint(QPainter *painter);
    void updateImage(uint8_t *imageData, const int &imageWidth, const int &imageHeight);
    void updateImage(const QByteArray& imageData, const QSize& imageSize);
    void updateImage(uint32_t *imageData, const int &imageWidth, const int &imageHeight);
    QImage* image();
    QImage* paintImage();
    Q_INVOKABLE void fit(int destWidth, int destHeight);

    Q_PROPERTY(QVariant colorTable READ getColorTable WRITE setColorTable NOTIFY colorTableChanged);
    Q_PROPERTY(bool showColorBar READ getShowColorBar WRITE setShowColorBar NOTIFY showColorBarChanged);
    Q_PROPERTY(bool colorTableInvert READ getColorTableInvert WRITE setColorTableInvert NOTIFY colorTableInvertChanged)
    Q_PROPERTY(int grayLow READ getGrayLow WRITE setGrayLow NOTIFY grayLowChanged);
    Q_PROPERTY(int grayHigh READ getGrayHigh WRITE setGrayHigh NOTIFY grayHighChanged);
    Q_PROPERTY(int zoom READ getZoom WRITE setZoom NOTIFY zoomChanged);

    QVariant getColorTable();

    bool getColorTableInvert();
    void setColorTableInvert(bool invert);

signals:
    void showColorBarChanged(bool showColorBar);
    void zoomChanging(int zoom);
    void zoomChanged(int zoom);

    void colorTableChanged();
    void colorTableInvertChanged();
    void grayLowChanged();
    void grayHighChanged();

public slots:

protected:
    void wheelEvent(QWheelEvent *event);

private:
    QImage* m_image = nullptr; // 外部传入的原始的图片
    QImage* m_paintImage = nullptr; // 最终呈现的图, 在原始图片上画了一些东西，比如 ColorBar
    QVector<QRgb> m_colorTable;
    QVector<QRgb> m_colorTableOriginal;
    bool m_showColorBar = false;
    bool m_colorTableInvert = false;
    int m_grayLow = 0;
    int m_grayHigh = 65535;
    int m_zoom = 100;
    bool m_isRGBImage = false;

    void setColorTable(QVariant colorTable);
    bool getShowColorBar();
    void setShowColorBar(bool showColorBar);
    int getGrayLow();
    void setGrayLow(int grayLow);
    int getGrayHigh();
    void setGrayHigh(int grayHigh);
    int getZoom();
    void setZoom(int zoom);
};

extern WzImageView* g_imageView;

#endif // WZIMAGE_VIEW_H
