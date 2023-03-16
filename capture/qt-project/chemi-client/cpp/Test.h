#ifndef TEST_H
#define TEST_H

#include <memory>
#include <vector>

#include <QObject>
#include <QFile>
#include <QDebug>
#include "WzImageService.h"
#include "WzImageFilter.h"
#include "WzFileDownloader.h"
#include "cpp/ziputils.h"

void TestCreateThumb();
void TestImageFilter();
void TestFileDownloader();
void TestCreateZipFile();

class WzTest: public QObject
{
    Q_OBJECT

public:
    WzTest(QObject *parent = nullptr);

    Q_INVOKABLE void testFileDownloader();

private:
    WzFileDownloader *fd = nullptr;

private slots:
    void fileDownloadFinished(const int &id);
};

#endif // TEST_H
