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

void TestFileDownloader()
{

}

WzTest::WzTest(QObject *parent) : QObject(parent)
{

}

void WzTest::testFileDownloader()
{
    fd = new WzFileDownloader;
    fd->setServerAddress("192.168.1.109");
    QStringList remoteFiles, localFiles;
    remoteFiles << "C:/Users/pipiluu/Desktop/2020-09-09/2020-09-09_22.20.34.tif";
    remoteFiles << "Z:/Users/pipiluu/chemi_images/2020-10-14/.shst_thumb/2020-10-14_17.24.13.jpg";
    localFiles << "/Users/pipiluu/downloader.tif";
    localFiles << "/Users/pipiluu/downloader2.jpg";
    fd->downloadFile(remoteFiles, localFiles);
}

void WzTest::fileDownloadFinished(const int &id)
{
    fd->deleteLater();
}

void TestCreateZipFile()
{
    ZipUtils* zipUtils = new ZipUtils();
    QStringList fileNames;

    fileNames << "E:/Users/SHST/Desktop/a/b_dark.jpg";
    fileNames << "E:/Users/SHST/Desktop/a/b_dark_16bit.tif";
    fileNames << "E:/Users/SHST/Desktop/a/b_dark_24bit.tif";
    fileNames << "E:/Users/SHST/Desktop/a/b_dark_8bit.tif";
    fileNames << "E:/Users/SHST/Desktop/a/b_marker.jpg";
    fileNames << "E:/Users/SHST/Desktop/a/b_marker_16bit.tif";
    fileNames << "E:/Users/SHST/Desktop/a/b_marker_24bit.tif";
    fileNames << "E:/Users/SHST/Desktop/a/b_marker_8bit.tif";
    fileNames << "E:/Users/SHST/Desktop/a/b_overlap.jpg";
    fileNames << "E:/Users/SHST/Desktop/a/b_overlap_24bit.tif";
    fileNames << "E:/Users/SHST/Desktop/a/b_overlap_8bit.tif";

    zipUtils->createZipFile(fileNames, "E:/Users/SHST/Desktop/a/b.zip");
//    while(zipUtils.runningCount() > 0) {
//        qDebug() << "Wait creating zip file";
//    }
}
