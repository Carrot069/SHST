#ifndef WZDISKUTILS_H
#define WZDISKUTILS_H

#include <QObject>
#include <QDebug>
#include <QStorageInfo>
#include <QList>

#include "WzDatabaseService.h"

class WzDiskUtils : public QObject
{
    Q_OBJECT
public:
    explicit WzDiskUtils(QObject *parent = nullptr);

    bool diskFreeSpaceLessThan(const QString &path, const int &MB);
    void clearHistoryImages(WzDatabaseService &dbService, const QString &path, const int &targetFreeSpace);
signals:

private:
    int getFileSize(QString filename);
    void deleteFile(QString filename);
};

#endif // WZDISKUTILS_H
