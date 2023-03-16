#include "WzDiskUtils.h"

WzDiskUtils::WzDiskUtils(QObject *parent) : QObject(parent)
{

}

bool WzDiskUtils::diskFreeSpaceLessThan(const QString &path, const int &MB)
{
    QStorageInfo si(path);
    if (si.isValid()) {
        qInfo() << "free space: " << si.bytesFree();
        return si.bytesFree() / 1024 / 1024 < MB;
    }
    return false;
}

void WzDiskUtils::clearHistoryImages(WzDatabaseService &dbService,
                                     const QString &path,
                                     const int &targetFreeSpace)
{
    // 清理的几种情况
    // X = 用户设定的小于这个值时开始清理图片
    // Y = 用户设定目标空闲空间
    // 1. 没有任何图片文件, 可用空间仍然小于X, 直接退出过程
    // 2. 有图片文件, 全部清理后可用空间仍然小于X, 退出过程
    // 3. 有图片文件, 清理至可用空间 >= Y

    QStringList deletedFileNames;
    QStorageInfo si(path);

    int64_t willClearSize = static_cast<uint64_t>(targetFreeSpace) * 1024 * 1024 - si.bytesFree(); // 需要清理的大小
    if (willClearSize <= 0)
        return;

    QList<QVariant> images = dbService.getImages();
    if (images.count() == 0)
        return;

    int64_t filesSize = 0;
    for (int i = 0; i < images.count(); i++) {
        QVariantMap imageInfoMap = images.at(i).toMap();

        QString imageFile = imageInfoMap["imageFile"].toString();
        QString imageThumbFile = imageInfoMap["imageThumbFile"].toString();
        QString imageWhiteFile = imageInfoMap["imageWhiteFile"].toString();

        QStorageInfo currentFileSI(imageFile);
        if (si.rootPath() != currentFileSI.rootPath())
            continue;

        filesSize += getFileSize(imageFile);
        filesSize += getFileSize(imageThumbFile);
        filesSize += getFileSize(imageWhiteFile);

        if (filesSize < willClearSize) {
            deleteFile(imageFile);
            deleteFile(imageThumbFile);
            deleteFile(imageWhiteFile);
            deletedFileNames.append(imageFile);
        } else {
            break;
        }
    }

    foreach(QString fn, deletedFileNames) {
        dbService.deleteImageByFileName(fn);
    }
}

int WzDiskUtils::getFileSize(QString filename)
{
    return QFileInfo(filename).size();
}

void WzDiskUtils::deleteFile(QString filename)
{
    QFile::remove(filename);
}
