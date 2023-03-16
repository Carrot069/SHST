#include <QCoreApplication>
#include <QDebug>
#include <QImage>
#include <QStringList>
#include <QFileInfo>
#include <QFile>

#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (argc <= 1) {
        std::cout << "Usage: Filename of image";
        return 0;
    }

    std::cout << argv[1];

    QFileInfo fi(argv[1]);
    QFile f(fi.absoluteFilePath() + ".txt");
    if (f.open(QIODevice::WriteOnly | QFile::Text)) {
        QTextStream ts(&f);
        QImage img(argv[1]);
        for (int i = 0; i < 256; i++) {
             QRgb rgb = img.pixel(i, 0);
             ts << QString::number(qRed(rgb)) << " " << QString::number(qGreen(rgb)) << " " << QString::number(qBlue(rgb)) << "\n";
        }
        f.close();
    } else {
        std::cout << "Open file failure";
    }

    return a.exec();
}
