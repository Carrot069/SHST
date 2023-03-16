#ifndef WZIMAGEDATABASE_H
#define WZIMAGEDATABASE_H

#include <QObject>
#include <QString>
#include <QTime>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QDir>
#include <QtDebug>
#include <QCoreApplication>
#include <QList>
#include <QFile>
#include <QStandardPaths>
#include <QSettings>

#include "WzGlobalEnum.h"
#include "WzGlobalConst.h"
#include "WzSetting2.h"

#ifdef HARDLOCK
#include "RY3_API.h"
#endif

class WzDatabaseService: public QObject
{
    Q_OBJECT
private:
    static void appendSql(QMap<QString, QVariant> imageInfo, const QString keyName, const QString fieldName, QString& sql);
    static QString getDataFileName();

public:
    explicit WzDatabaseService(QObject *parent = nullptr);
    ~WzDatabaseService() override;

    bool connect();
    bool initTables();
    bool disconnect();
    bool deleteImage(const QString& imageFile);
    // 将json格式的图片信息更新到数据库
    Q_INVOKABLE bool saveImage(QVariantMap info);
    // 从数据库表中读取N张图片的记录
    Q_INVOKABLE QList<QVariant> getImages(const uint start = 0, const uint count = 100);

    // 根据文件名读取图片信息
    Q_INVOKABLE QVariantMap getImageByFileName(const QString& fileName);
    // 根据文件名从数据库中删除信息
    Q_INVOKABLE void deleteImageByFileName(const QString& fileName);

    static int getTableFieldNames(QString tableName, QStringList& fieldNames);
    static int addFields(QString tableName, QMap<QString, QString> fields);

    // 保存和读取选项
    Q_INVOKABLE int readIntOption(const QString& optionName, const int defaultValue);
    Q_INVOKABLE void saveIntOption(const QString& optionName, const int value);
    Q_INVOKABLE QString readStrOption(const QString& optionName, const QString& defaultValue);
    Q_INVOKABLE void saveStrOption(const QString& optionName, const QString& value);

    // 保存和读取高级设置选项
    Q_INVOKABLE void saveAdminSetting(QVariantMap params);
    Q_INVOKABLE QVariantMap readAdminSetting();

private:
    QVariantMap queryToMap(const QSqlQuery& query);
    void readIni();
};

#endif // WZIMAGEDATABASE_H
