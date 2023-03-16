#ifndef WZ_PSEUDO_COLOR_H
#define WZ_PSEUDO_COLOR_H

#include <QtQuick>
#include <QSize>

class WzPseudoColor : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit WzPseudoColor(QQuickItem *parent = nullptr);
    ~WzPseudoColor();

    void paint(QPainter *painter);

    Q_PROPERTY(QVariant rgbList READ getRgbList WRITE setRgbList);
    Q_PROPERTY(bool vertical READ getVertical WRITE setVertical NOTIFY verticalChanged);
signals:
    bool verticalChanged(bool vertical);

public slots:

private:
    QImage* m_image;
    QVariant m_rgbList;
    bool m_vertical = false;

    void updateImage();

    QVariant getRgbList();
    void setRgbList(QVariant rgbList);

    bool getVertical();
    void setVertical(bool vertical);
};

#endif // WZ_PSEUDO_COLOR_H
