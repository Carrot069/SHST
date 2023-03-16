#include "Test.h"

void TestCreateThumb() {
    WzImageBuffer imageBuffer;
    WzImageService::loadImage("D:/PiPiLuu/code/_project/chemi/qt-project/chemi/example.tif", &imageBuffer);
    imageBuffer.update();

    WzImageBuffer thumbBuffer;
    thumbBuffer.width = 120;
    thumbBuffer.height = 120;

    WzImageService::createThumb(thumbBuffer, imageBuffer, 200, false);
    WzImageService::saveImageAsTiff(thumbBuffer, "D:/thumb.tif");

    delete []imageBuffer.buf;
    delete []thumbBuffer.buf;
}

void TestImageFilter()
{
    WzImageFilter filter;
    WzImageBuffer buffer;
    WzImageService::loadImage("/Users/pipiluu/Code/_project/chemi/qt-project/demo-data/2020-05-09 11.04.54.tif", &buffer);
    buffer.update();

//    WzImageBuffer newBuffer;
//    newBuffer.buf = new uint8_t[(buffer.width + 2) * (buffer.height + 2) * 2];
//    newBuffer.update();

//    filter.add4Line(buffer.bit16Array, newBuffer.bit16Array, buffer.width, buffer.height);
//    newBuffer.width = buffer.width + 2;
//    newBuffer.height = buffer.height + 2;
//    newBuffer.bitDepth = 16;

//    WzImageService::saveImageAsTiff(newBuffer, "/Users/pipiluu/Code/_project/chemi/qt-project/chemi/example.add4Line.tif");

    //filter.medianFilter(buffer.bit16Array, buffer.width, buffer.height, 300);
    filter.filterLightspot(buffer.bit16Array, buffer.width, buffer.height);
    WzImageService::saveImageAsTiff(buffer, "/Users/pipiluu/Code/_project/chemi/qt-project/demo-data/2020-05-09 11.04.54.filtered2.tif");

//    WzImageBuffer remove4LineBuffer;
//    remove4LineBuffer.buf = new uint8_t[buffer.width * buffer.height * 2];
//    remove4LineBuffer.update();
//    remove4LineBuffer.width = buffer.width;
//    remove4LineBuffer.height = buffer.height;
//    remove4LineBuffer.bitDepth = 16;
//    filter.remove4Line(newBuffer.bit16Array, remove4LineBuffer.bit16Array, newBuffer.width, newBuffer.height);

//    WzImageService::saveImageAsTiff(remove4LineBuffer, "/Users/pipiluu/Code/_project/chemi/qt-project/chemi/example.remove4Line.tif");

    delete []buffer.buf;
    //delete []newBuffer.buf;
    //delete []remove4LineBuffer.buf;
}

void TestFitRect() {
    double scale = 100;
    qDebug() << "TestFitRect:";
    qDebug() << WzUtils::fitRect(QRect(0, 0, 200, 200), QRect(0, 0, 100, 100), scale, true, true) << ", scale:" << scale;
    qDebug() << WzUtils::fitRect(QRect(0, 0, 200, 200), QRect(0, 0, 300, 300), scale, true, true) << ", scale:" << scale;
    qDebug() << WzUtils::fitRect(QRect(0, 0, 200, 200), QRect(0, 0, 300, 100), scale, true, true) << ", scale:" << scale;
    qDebug() << WzUtils::fitRect(QRect(0, 0, 200, 200), QRect(0, 0, 100, 300), scale, true, true) << ", scale:" << scale;

    qDebug() << WzUtils::fitRect(QRect(0, 0, 200, 200), QRect(0, 0, 100, 100), scale, true, false) << ", scale:" << scale;
    qDebug() << WzUtils::fitRect(QRect(0, 0, 200, 200), QRect(0, 0, 400, 400), scale, true, false) << ", scale:" << scale;
    qDebug() << "Test end";
}
