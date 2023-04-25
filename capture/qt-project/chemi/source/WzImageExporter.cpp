#include "WzImageExporter.h"

WzImageExporter::WzImageExporter(QObject *parent) : QObject(parent)
{

}

void WzImageExporter::exportImages(const QVariantMap &params)
{
    if (!checkExportPath(params["exportPath"].toString())) {
        qWarning() << "ImageExporter, export path not found";
        return;
    }

    if (!checkExportFormat(params["exportFormat"].toString()))
        return;

    QVariantList images = params["images"].toList();
    for (int i = 0; i < images.count(); i++) {
        QVariantMap image = images[i].toMap();
        exportImage(image);
    }
}

bool WzImageExporter::getImagesWhite() const
{
    return m_ImagesWhite;
}
void WzImageExporter::setImagesWhite(bool ImagesWhite)
{
    m_ImagesWhite=ImagesWhite;
    qDebug()<<"批量图"<<m_ImagesWhite;
    emit ImagesWhiteChanged();
}

void WzImageExporter::test()
{
    QJsonObject testData;

    testData["exportPath"] = "/Users/pipiluu/export_test/";
    testData["exportFormat"] = "jpeg";

    QJsonArray images;

    QJsonObject image;
    image["imageFile"] = "/Users/pipiluu/downloader.tif";
    image["grayLow"] = 0;
    image["grayHigh"] = 20000;

    images.append(image);

    testData["images"] = images;

    WzImageExporter exporter;
    exporter.exportImages(testData.toVariantMap());
}


void WzImageExporter::exportImage(const QVariantMap &image)
{
    QString imageFile = image["imageFile"].toString();
    int grayLow = image["grayLow"].toInt();
    int grayHigh = image["grayHigh"].toInt();
    QString exportFormat = image["exportFormat"].toString();

    QString destFileName = makeDestFileName(image);
    if (QFileInfo::exists(destFileName)) {
        qWarning() << "ImageExporter, file is exists, " << destFileName;
        return;
    }

    WzImageView view;
    WzImageService is;
    is.setImageView(&view);
    is.openImage(imageFile);
    if (0 == grayHigh) {
        is.calculateLowHigh();
        grayHigh = is.getHigh() / 2;
    }
    is.updateLowHigh(grayLow, grayHigh);
    is.updateView();
    is.setChangedWhite(getImagesWhite());
    if ("jpeg" == exportFormat || "jpg" == exportFormat || "png" == exportFormat) {
        QPainter painter;
        view.setColorTableInvert(true);
        view.paint(&painter);
    }
    QJsonObject saveAsParams;
    saveAsParams["fileName"] = destFileName;
    saveAsParams["format"] = exportFormat;
    is.saveAsImage(saveAsParams);
}

bool WzImageExporter::checkExportFormat(const QString &format)
{
    if ("tiff16" == format ||
            "tiff8" == format ||
            "jpeg" == format ||
            "jpg" == format ||
            "png" == format) {
        return true;
    }
    return false;
}

QString WzImageExporter::makeDestFileName(const QVariantMap &image)
{
    // <NewPath><OldBaseName><NewSuffix>

    QString sampleName = image["sampleName"].toString();
    QString oldBaseName;
    if (sampleName == "")
        oldBaseName = QFileInfo(image["imageFile"].toString()).completeBaseName();
    else
        oldBaseName = sampleName;

    QString format = image["exportFormat"].toString();
    QString suffix;
    if ("tiff16" == format || "tiff8" == format)
        suffix = ".tif";
    else if ("jpeg" == format || "jpg" == format)
        suffix = ".jpg";
    else if ("png" == format)
        suffix = ".png";

    QUrl localDir(image["exportPath"].toString());
    QDir exportPath(localDir.toLocalFile());
    QFileInfo destFileName(
                exportPath,
                oldBaseName + suffix);

    if (destFileName.exists()) {
        int fileNumber = 2;
        do {
            destFileName = QFileInfo (
                        exportPath,
                        QString("%1(%2)%3").arg(oldBaseName).arg(fileNumber).arg(suffix));
            fileNumber++;
        } while(destFileName.exists());
    }

    return destFileName.absoluteFilePath();
}

bool WzImageExporter::checkExportPath(const QString &path)
{
    QUrl url(path);
    QDir dir(url.toLocalFile());
    return dir.exists();
}
