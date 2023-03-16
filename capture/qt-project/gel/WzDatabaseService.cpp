#include "WzDatabaseService.h"

int WzDatabaseService::instanceCount = 0; // 实例数量
QSqlDatabase WzDatabaseService::db = QSqlDatabase::addDatabase("QSQLITE");

WzDatabaseService::WzDatabaseService(QObject *parent):
    QObject(parent)
{
    qDebug() << "WzDatabaseService created";
    if (0 == instanceCount)
        connect();
    instanceCount++;

    readIni();
}

WzDatabaseService::~WzDatabaseService() {
    instanceCount--;
    if(instanceCount <= 0)
        db.close();
    qDebug() << "~WzDatabaseService";
}

bool WzDatabaseService::connect() {
    if (db.isOpen())
        return true;

    QString dbFile = getDataFileName();
    qInfo() << "Database File:" << dbFile;
    db.setDatabaseName(dbFile);
    if (!db.open()) {
        // TODO 处理数据库打开失败
        qWarning() << "数据库打开失败, " << qPrintable(db.lastError().text());
        return false;
    }

    initTables();

    return true;
};

bool WzDatabaseService::disconnect() {
    if (db.isOpen())
        db.close();
    return true;
};

bool WzDatabaseService::initTables() {
    qDebug() << "WzDatabaseService::initTables";
    QSqlQuery query;
    // TODO binning, camera_temperature, machine_sn
    bool b = query.exec("create table if not exists images("
                        "id integer primary key autoincrement, "
                        "image_file text,"       // 主图完整路径文件名
                        "image_thumb_file text," // 缩略图完整路径文件名
                        "image_white_file text," // 白光图完整路径文件名
                        "image_source int,"      // 图片来源(拍摄/打开)
                        "create_date datetime,"  // 记录产生日期
                        "update_date datetime,"  // 更新记录的日期
                        "capture_date datetime," // 拍摄日期
                        "exposure_ms int,"       // 曝光毫秒数
                        "sample_name text,"      // 样品名, 用户输入
                        "gray_low int,"          // 界面中调整后的 low 值
                        "gray_high int,"         // 界面中调整后的 high 值
                        "gray_max int,"          // 图片的最大灰阶
                        "gray_min int,"          // 图片的最小灰阶
                        "group_id int"           // 多帧拍摄时用到此字段，用2000年以来的毫秒数作为字段值
                        ")");

    QMap<QString, QString> newFields;
    newFields.clear();
    newFields["binning"] = "int";
    newFields["camera_temperature"] = "real";   // 拍摄时相机温度
    newFields["machine_sn"] = "text";           // 仪器序列号
    newFields["image_invert"] = "int";          // 图片是否反色显示, 0x01 代表反色, 0x02 不反色, 其他值代表默认值: 反色
    newFields["opened_light"] = "text";         // 拍摄时开着的光源, 比如紫外透射, 如果没开任何光源则为空
    addFields("images", newFields);

    if (b) {
        b = query.exec("create table if not exists options("
                          "id integer primary key autoincrement, "
                          "option_name varchar(512), "
                          "option_int int, "
                          "option_datetime datetime, "
                          "option_text text, "
                          "option_real real"
                          ")");
    }
    return b;
}

bool WzDatabaseService::saveImage(QVariantMap imageInfo) {
    qDebug() << "WzDatabaseService::saveImage";
    if (imageInfo.empty()) return false;
    if (!imageInfo.contains("imageFile")) return false;
    if (imageInfo["imageFile"].toString() == "") return false;

    QSqlQuery query;
    query.prepare("select * from images where image_file = ?");
    query.addBindValue(imageInfo["imageFile"]);
    query.exec();
    if (!query.next()) {
        QSqlQuery insertQuery(db);
        insertQuery.prepare("insert into images (image_file, create_date)values(?,?)");
        insertQuery.addBindValue(imageInfo["imageFile"].toString());
        insertQuery.addBindValue(QDateTime::currentDateTime());
        if (!insertQuery.exec()) {
            qWarning() << "saveImage, insert into error";
            return false;
        }
    }

    QString updateSql = "";

    appendSql(imageInfo, "imageThumbFile" , "image_thumb_file", updateSql);
    appendSql(imageInfo, "imageWhiteFile" , "image_white_file", updateSql);
    appendSql(imageInfo, "imageSource"    , "image_source"    , updateSql);
    appendSql(imageInfo, "createDate"     , "create_date"     , updateSql);
    appendSql(imageInfo, "updateDate"     , "update_date"     , updateSql);
    appendSql(imageInfo, "captureDate"    , "capture_date"    , updateSql);
    appendSql(imageInfo, "exposureMs"     , "exposure_ms"     , updateSql);
    appendSql(imageInfo, "sampleName"     , "sample_name"     , updateSql);
    appendSql(imageInfo, "grayLow"        , "gray_low"        , updateSql);
    appendSql(imageInfo, "grayHigh"       , "gray_high"       , updateSql);
    appendSql(imageInfo, "grayMax"        , "gray_max"        , updateSql);
    appendSql(imageInfo, "grayMin"        , "gray_min"        , updateSql);
    appendSql(imageInfo, "groupId"        , "group_id"        , updateSql);
    appendSql(imageInfo, "imageInvert"    , "image_invert"    , updateSql);
    appendSql(imageInfo, "openedLight"    , "opened_light"    , updateSql);

    if ("" == updateSql)
        return false;

    QSqlQuery updateQuery(db);
    updateSql = "update images set " + updateSql + " where image_file = ?";
    updateQuery.prepare(updateSql);

    if (imageInfo.contains("imageThumbFile")) updateQuery.addBindValue(imageInfo["imageThumbFile"]);
    if (imageInfo.contains("imageWhiteFile")) updateQuery.addBindValue(imageInfo["imageWhiteFile"]);
    if (imageInfo.contains("imageSource"   )) updateQuery.addBindValue(imageInfo["imageSource"   ]);
    if (imageInfo.contains("createDate"    )) updateQuery.addBindValue(imageInfo["createDate"    ]);
    if (imageInfo.contains("updateDate"    )) updateQuery.addBindValue(imageInfo["updateDate"    ]);
    if (imageInfo.contains("captureDate"   )) updateQuery.addBindValue(imageInfo["captureDate"   ]);
    if (imageInfo.contains("exposureMs"    )) updateQuery.addBindValue(imageInfo["exposureMs"    ]);
    if (imageInfo.contains("sampleName"    )) updateQuery.addBindValue(imageInfo["sampleName"    ]);
    if (imageInfo.contains("grayLow"       )) updateQuery.addBindValue(imageInfo["grayLow"       ]);
    if (imageInfo.contains("grayHigh"      )) updateQuery.addBindValue(imageInfo["grayHigh"      ]);
    if (imageInfo.contains("grayMax"       )) updateQuery.addBindValue(imageInfo["grayMax"       ]);
    if (imageInfo.contains("grayMin"       )) updateQuery.addBindValue(imageInfo["grayMin"       ]);
    if (imageInfo.contains("groupId"       )) updateQuery.addBindValue(imageInfo["groupId"       ]);
    if (imageInfo.contains("imageInvert"   )) updateQuery.addBindValue(imageInfo["imageInvert"   ]);
    if (imageInfo.contains("openedLight"   )) updateQuery.addBindValue(imageInfo["openedLight"   ]);

    updateQuery.addBindValue(imageInfo["imageFile"]);

    updateQuery.exec();

    return true;
}

bool WzDatabaseService::deleteImage(const QString& imageFile) {
    qDebug() << "WzDatabaseService::deleteImage";
    QSqlQuery query;
    query.prepare("delete from images where image_file = ?");
    query.addBindValue(imageFile);
    query.exec();
    return true;
}


QList<QVariant> WzDatabaseService::getImages(const uint start, const uint count) {
    Q_UNUSED(start)
    Q_UNUSED(count)
    qDebug() << "WzDatabaseService::getImages";
    QList<QVariant> results;
    QSqlQuery query;
    query.prepare("select * from images order by update_date");
    query.exec();
    qDebug() << query.record().count();
    while (query.next()) {
        if(QFile::exists(query.value("image_file").toString()))
            results.append(queryToMap(query));
    }
    return results;
}

// 根据文件名读取图片信息
QVariantMap WzDatabaseService::getImageByFileName(const QString& fileName) {
    qDebug() << "WzDatabaseService::getImageByFileName";
    QString localFile = "";

    QUrl fileUrl(fileName);
    if (fileUrl.isLocalFile())
        localFile = fileUrl.toLocalFile();
    else
        localFile = fileName;

    QSqlQuery query;
    query.prepare("select * from images "
                  "where image_file = ?");
    query.addBindValue(localFile);
    query.exec();

    if (query.next()) {
        return queryToMap(query);
    } else {
        return QVariantMap();
    }
}

void WzDatabaseService::deleteImageByFileName(const QString &fileName) {
    qDebug() << "WzDatabaseService::deleteImageByFileName";
    QString localFile = "";

    QUrl fileUrl(fileName);
    if (fileUrl.isLocalFile())
        localFile = fileUrl.toLocalFile();
    else
        localFile = fileName;

    QSqlQuery query;
    query.prepare("delete from images "
                  "where image_file = ?");
    query.addBindValue(localFile);
    query.exec();
}

int WzDatabaseService::getTableFieldNames(QString tableName, QStringList& fieldNames) {
    QSqlQuery query;
    query.exec("select * from " + tableName);
    QSqlRecord record = query.record();
    for (int i = 0; i < record.count(); i++) {
        fieldNames.append(record.fieldName(i));
    }
    return record.count();
}

int WzDatabaseService::addFields(QString tableName, QMap<QString, QString> fields) {
    QStringList existedFields;
    getTableFieldNames(tableName, existedFields);

    QString sql;
    sql = "ALTER TABLE " + tableName + " ADD COLUMN ";
    QSqlQuery query;
    int idx = 0;
    QMapIterator<QString, QString> iter(fields);
    while (iter.hasNext()) {
        iter.next();
        if (existedFields.indexOf(iter.key()) == -1) {
            bool b = query.exec("ALTER TABLE " + tableName + " ADD COLUMN " + iter.key() + " " + iter.value());
            if (!b)
                qWarning() << "addFields error:" << query.lastError();
            idx++;
        }
    }
    return idx + 1;
};

QVariantMap WzDatabaseService::queryToMap(const QSqlQuery& query) {
    QMap<QString, QVariant> imageInfoMap;

    imageInfoMap["imageFile"     ] = query.value("image_file"      );
    imageInfoMap["imageThumbFile"] = query.value("image_thumb_file");
    imageInfoMap["imageWhiteFile"] = query.value("image_white_file");
    imageInfoMap["imageSource"   ] = query.value("image_source"    ).toInt();
    imageInfoMap["createDate"    ] = query.value("create_date"     );
    imageInfoMap["updateDate"    ] = query.value("update_date"     );
    imageInfoMap["captureDate"   ] = query.value("capture_date"    );
    imageInfoMap["exposureMs"    ] = query.value("exposure_ms"     ).toInt();
    imageInfoMap["sampleName"    ] = query.value("sample_name"     );
    imageInfoMap["grayLow"       ] = query.value("gray_low"        );
    imageInfoMap["grayHigh"      ] = query.value("gray_high"       );
    imageInfoMap["grayMax"       ] = query.value("gray_max"        );
    imageInfoMap["grayMin"       ] = query.value("gray_min"        );
    imageInfoMap["groupId"       ] = query.value("group_id"        ).toInt();
    imageInfoMap["imageInvert"   ] = query.value("image_invert"    ).toInt();
    imageInfoMap["opened_light"  ] = query.value("opened_light"    );

    return imageInfoMap;
}

void WzDatabaseService::readIni()
{
    QString iniFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + kIniFileName;
    QSettings settings(iniFile, QSettings::IniFormat);
    QString language = settings.value("Normal/languageName").toString();
    if (language != "") {
        saveStrOption("languageName", language);
        settings.remove("Normal/languageName");
    }
}

void WzDatabaseService::saveIntOption(const QString& optionName, const int value) {
    QSqlQuery query;
    query.prepare("select * from options where option_name = ?");
    query.addBindValue(optionName);
    query.exec();
    if (query.next()) {
        QSqlQuery updateQuery;
        updateQuery.prepare("update options set option_int = ? where option_name = ?");
        updateQuery.addBindValue(value);
        updateQuery.addBindValue(optionName);
        updateQuery.exec();
    } else {
        QSqlQuery insertQuery;
        insertQuery.prepare("insert into options (option_name, option_int)values(?,?)");
        insertQuery.addBindValue(optionName);
        insertQuery.addBindValue(value);
        insertQuery.exec();
    }
};

int WzDatabaseService::readIntOption(const QString& optionName, const int defaultValue) {
    QSqlQuery query;
    query.prepare("select option_int from options where option_name = ?");
    query.addBindValue(optionName);
    query.exec();
    if (query.next()) {
        return query.value(0).toInt();
    } else {
        return  defaultValue;
    }
};

void WzDatabaseService::saveStrOption(const QString& optionName, const QString& value) {
    QSqlQuery query;
    query.prepare("select * from options where option_name = ?");
    query.addBindValue(optionName);
    query.exec();
    if (query.next()) {
        QSqlQuery updateQuery;
        updateQuery.prepare("update options set option_text = ? where option_name = ?");
        updateQuery.addBindValue(value);
        updateQuery.addBindValue(optionName);
        updateQuery.exec();
    } else {
        QSqlQuery insertQuery;
        insertQuery.prepare("insert into options (option_name, option_text)values(?,?)");
        insertQuery.addBindValue(optionName);
        insertQuery.addBindValue(value);
        insertQuery.exec();
    }
};

QString WzDatabaseService::readStrOption(const QString& optionName, const QString& defaultValue) {
    QSqlQuery query;
    query.prepare("select option_text from options where option_name = ?");
    query.addBindValue(optionName);
    query.exec();
    if (query.next()) {
        return query.value(0).toString();
    } else {
        return defaultValue;
    }
};

void WzDatabaseService::appendSql(QMap<QString, QVariant> imageInfo, const QString keyName, const QString fieldName, QString& sql) {
    if (imageInfo.contains(keyName)) {
        if ("" == sql)
            sql = fieldName + " = ?";
        else
            sql += ", " + fieldName + " = ?";
    }
};

QString WzDatabaseService::getDataFileName() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + kDataFileName;
}

void WzDatabaseService::saveAdminSetting(QVariantMap params)
{
    qDebug() << "admin setting, " << params;
#ifdef HARDLOCK
    WzSetting2::saveAdminSetting(params);
#else
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(params);
    saveStrOption("admin_setting", jsonDoc.toJson());
#endif
}

QVariantMap WzDatabaseService::readAdminSetting()
{
#ifdef HARDLOCK
    QVariantMap params = WzSetting2::readAdminSetting();
#else
    QString jsonStr = readStrOption("admin_setting", "{}");
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
    QVariantMap params = jsonDoc.toVariant().toMap();
#endif
    if (params.count() == 0) {
        AdminSettingStruct buf;
        params = WzSetting2::buf2Map(buf);
    }
    return params;
};
