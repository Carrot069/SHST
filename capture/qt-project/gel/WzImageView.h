#ifndef WZIMAGE_VIEW_H
#define WZIMAGE_VIEW_H

#include <QtQuick>
#include <QSize>
#include <QFont>
#include <QFontMetrics>

#include "exiv2/exiv2.hpp"

#include "WzGlobalEnum.h"
#include "WzImageBuffer.h"
#include "WzUtils.h"

class WzImageView : public QQuickPaintedItem
{
    Q_OBJECT

public:
    static void drawColorBar(QImage& image, int grayLow, int grayHigh, QVariant rgbList);

    explicit WzImageView(QQuickItem *parent = nullptr);
    ~WzImageView();

    void paint(QPainter *painter) override;
    void updateImage(uint8_t *imageData, const int &imageWidth, const int &imageHeight, WzImageBuffer *imageBuffer);
    void updateImage(const QByteArray& imageData, const QSize& imageSize, WzImageBuffer *imageBuffer);
    void updateImage(uint32_t *imageData, const int &imageWidth, const int &imageHeight, WzImageBuffer *imageBuffer);
    QImage* image();
    QImage* paintImage();
    QVariant getColorTable();
    Q_INVOKABLE void fit(int destWidth, int destHeight);

    Q_INVOKABLE void cropImageApply();
    Q_INVOKABLE void cropImageCancel();
    Q_INVOKABLE void cropImageReselect();

    Q_PROPERTY(QVariant colorTable READ getColorTable WRITE setColorTable NOTIFY colorTableChanged);
    Q_PROPERTY(bool showColorBar READ getShowColorBar WRITE setShowColorBar NOTIFY showColorBarChanged);
    Q_PROPERTY(bool colorTableInvert READ getColorTableInvert WRITE setColorTableInvert NOTIFY colorTableInvertChanged)
    Q_PROPERTY(int grayLow READ getGrayLow WRITE setGrayLow NOTIFY grayLowChanged);
    Q_PROPERTY(int grayHigh READ getGrayHigh WRITE setGrayHigh NOTIFY grayHighChanged);
    Q_PROPERTY(int zoom READ getZoom WRITE setZoom NOTIFY zoomChanged);
signals:
    void showColorBarChanged(bool showColorBar);
    void zoomChanged(int zoom);

    void colorTableChanged();
    void colorTableInvertChanged();
    void grayLowChanged();
    void grayHighChanged();

public:
    Q_PROPERTY(int imageWidth READ getImageWidth NOTIFY imageWidthChanged)
    Q_PROPERTY(int imageHeight READ getImageHeight NOTIFY imageHeightChanged)
    Q_PROPERTY(int showWidth READ getShowWidth NOTIFY showWidthChanged)
    Q_PROPERTY(int showHeight READ getShowHeight NOTIFY showHeightChanged)
    Q_PROPERTY(WzEnum::AnalysisAction action READ getAction WRITE setAction NOTIFY actionChanged);
    Q_PROPERTY(QRect selectedRect READ getSelectedRect)
    Q_PROPERTY(QRect selectedRectNoZoom READ getSelectedRectNoZoom)
protected:
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
private:
    WzEnum::AnalysisAction m_action = WzEnum::AnalysisActionNone;
    WzImageBuffer *m_imageBuffer = nullptr; // 分析用
    int m_showWidth = 1;
    int m_showHeight = 1;
    //WzAnalysisAreaList *m_areas;
    QPoint m_mousePressPoint;
    QPoint m_mouseMovePos;
    QPoint m_mousePressPosNoZoom;
    QPoint m_mouseMovePosNoZoom;
    bool m_isMousePressed;
    double m_imageRotateAngle = 0;
    bool m_isCropImageSelected = false;
    QRect m_imageSrcRect;
    QRect m_paintDstRect;
    int getShowWidth() const;
    int getShowHeight() const;
    WzEnum::AnalysisAction getAction() const;
    void setAction(const WzEnum::AnalysisAction &action);
    int getImageWidth() const;
    int getImageHeight() const;
    QRect getSelectedRect();
    QRect getSelectedRectNoZoom();
    void showSizeChange();
    void drawColorBar(QPainter *painter);
signals:
    void imageRotate(const double& angle);
    void cropImageSelected(const QRect &rect);
    void cropImageUnselect();
    void cropImageRectMoving(const QRect &rect);
    void currentGrayChanged(const int x, const int y, const int gray);
    void imageWidthChanged(int imageWidth);
    void imageHeightChanged(int imageHeight);
    void showWidthChanged(int showWidth);
    void showHeightChanged(int showHeight);
    void actionChanged(const WzEnum::AnalysisAction &action);
    void popupMenu(const int &x, const int &y);
    void zoomChanging(int zoom);

public slots:

protected:
    void wheelEvent(QWheelEvent *event) override;

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
    bool getColorTableInvert();
    void setColorTableInvert(bool invert);
    int getGrayLow();
    void setGrayLow(int grayLow);
    int getGrayHigh();
    void setGrayHigh(int grayHigh);
    int getZoom();
    void setZoom(int zoom);
};

extern WzImageView* g_imageView;

#endif // WZIMAGE_VIEW_H
