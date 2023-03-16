#ifndef WZIMAGEEXPORTER_H
#define WZIMAGEEXPORTER_H

/********************************************************
  * 批量导出图片
  * 被导出图片须为TIFF 16bit
  * 可导出格式: TIFF16/8、JPEG、PNG
  * 导出为16位tiff时原样复制
  * 导出其他实际数据为8位的格式时, 根据传入的参数中的low和high转换成8位灰阶
********************************************************/

#include <QObject>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QPainter>

#include "WzImageService.h"

class WzImageExporter : public QObject
{
    Q_OBJECT
public:
    explicit WzImageExporter(QObject *parent = nullptr);

    /* 必须参数:
     * exportPath: 导出路径
     * exportFormat: tiff16/tiff8/jpeg/png
     * 参数格式:
     * {
     *   exportPath: "path",
     *   exportFormat: "tiff16",
     *   images: [
     *     {
     *       imageFile: "",
     *       low: 0,
     *       high: 20000
     *     },
     *     ...
     *   ]
     * }
     */
    Q_INVOKABLE void exportImages(const QVariantMap &params);

    static void test();

private:
    void exportImage(const QVariantMap &image);
    bool checkExportPath(const QString &path);
    bool checkExportFormat(const QString &format);
    QString makeDestFileName(const QVariantMap &image);

signals:
    void progress(const int &currentIndex, const int &total);
};

#endif // WZIMAGEEXPORTER_H
