#ifndef WZIMAGEFILTER_H
#define WZIMAGEFILTER_H

#include <QObject>
#include <QDebug>

class WzImageFilter
{
public:
    // 在四周增加4条1像素的边, 两个缓冲区都需要提前申请好内存
    void add4Line(uint16_t *buffer, uint16_t *newBuffer,
                  const int &imageWidth, const int &imageHeight);

    // 去掉四周4条1像素的边, 两个缓冲区都需要提前申请好内存
    void remove4Line(uint16_t *buffer, uint16_t *newBuffer,
                     const int &imageWidth, const int &imageHeight);

public:
    WzImageFilter();

    // 两个缓冲区都需要提前申请好内存
    void medianFilter(uint16_t *buffer,
                const int &imageWidth, const int imageHeight,
                const int threshold);

    // 去除孤立亮点
    void filterLightspot(uint16_t *buffer,
                         const int &imageWidth, const int imageHeight);
};

#endif // WZIMAGEFILTER_H
