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
    static int instanceCount; // 实例数量
    static QSqlDatabase db;

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
    Q_INVOKABLE bool readBoolOption(const QString& optionName, const bool defaultValue);
    Q_INVOKABLE void saveBoolOption(const QString& optionName, const bool value);
    Q_INVOKABLE int readIntOption(const QString& optionName, const int defaultValue);
    Q_INVOKABLE void saveIntOption(const QString& optionName, const int value);
    Q_INVOKABLE QString readStrOption(const QString& optionName, const QString& defaultValue);
    Q_INVOKABLE void saveStrOption(const QString& optionName, const QString& value);
    Q_INVOKABLE bool existsStrOption(const QString& optionName);

    // 保存和读取高级设置选项
    Q_INVOKABLE void saveAdminSetting(QVariantMap params);
    Q_INVOKABLE QVariantMap readAdminSetting();

private:
    QVariantMap queryToMap(const QSqlQuery& query);
    void readIni();

private:
    const QString ADV_SET_FILE = "AdvSet.ini";
    QString getAdvSetFullFile() const;
    bool checkAdvSetIni() const;
    bool deleteAdvSetIni() const;
public:
    Q_INVOKABLE void saveAdminSettingToDisk(QVariantMap params);
    Q_INVOKABLE QVariantMap readAdminSettingFromDisk() const;
    Q_INVOKABLE void saveValueToDisk(const QString& keyName, const QVariant value);
    Q_INVOKABLE QVariant readValueFromDisk(const QString& keyName, const QVariant defaultValue = QVariant());
};

#endif // WZIMAGEDATABASE_H
