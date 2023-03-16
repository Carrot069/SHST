#include "WzDatabaseService.h"

aes256_key WzDatabaseService::s_aesKey =
    {0x65, 0x85, 0x20, 0x30, 0xed, 0xe5, 0xa5, 0x64,
     0x0c, 0xc5, 0x49, 0xe2, 0x4d, 0xb9, 0x63, 0xcc,
     0x16, 0xeb, 0x2d, 0x0f, 0x3e, 0xd5, 0x5c, 0x4f,
     0x70, 0x4a, 0xc3, 0xbe, 0x60, 0xaa, 0x5f, 0xc0};

int WzDatabaseService::instanceCount = 0; // 实例数量
DbOwner *WzDatabaseService::dbOwner = nullptr;

WzDatabaseService::WzDatabaseService(QObject *parent):
    QObject(parent)
{
    qDebug() << "WzDatabaseService created";
    if (nullptr == dbOwner) {
        dbOwner = new DbOwner();
    }
    qDebug() << QSqlDatabase::drivers();
    qDebug() << QCoreApplication::libraryPaths();
    if (0 == instanceCount)
        connect();
    instanceCount++;
    readIni();
}

WzDatabaseService::~WzDatabaseService() {
    instanceCount--;
    if(instanceCount <= 0) {
        dbOwner->db.close();
        delete dbOwner;
        dbOwner = nullptr;
    }
    qDebug() << "~WzDatabaseService";
}

bool WzDatabaseService::connect() {
    if (dbOwner->db.isOpen())
        return true;

    QString dbFile = getDataFileName();
    qInfo() << "Database File:" << dbFile;
    dbOwner->db.setDatabaseName(dbFile);
    if (!dbOwner->db.open()) {
        // TODO 处理数据库打开失败
        qWarning() << "数据库打开失败, " << qPrintable(dbOwner->db.lastError().text());
        return false;
    }

    initTables();

    return true;
};

bool WzDatabaseService::disconnect() {
    if (dbOwner->db.isOpen())
        dbOwner->db.close();
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
    newFields["gray_low_marker"] = "int";       // low gray of marker
    newFields["gray_high_marker"] = "int";      // high gray of marker
    newFields["color_channel"] = "int";         // 颜色通道, 0 代表未叠加, 1/2/3 依次为 R/G/B
    newFields["palette"] = "text";              // 调色板
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
        QSqlQuery insertQuery(dbOwner->db);
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
    appendSql(imageInfo, "grayLowMarker"  , "gray_low_marker" , updateSql);
    appendSql(imageInfo, "grayHighMarker" , "gray_high_marker", updateSql);
    appendSql(imageInfo, "colorChannel"   , "color_channel"   , updateSql);
    appendSql(imageInfo, "palette"        , "palette"         , updateSql);

    if ("" == updateSql)
        return false;

    QSqlQuery updateQuery(dbOwner->db);
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
    if (imageInfo.contains("grayLowMarker" )) updateQuery.addBindValue(imageInfo["grayLowMarker" ]);
    if (imageInfo.contains("grayHighMarker")) updateQuery.addBindValue(imageInfo["grayHighMarker"]);
    if (imageInfo.contains("colorChannel"  )) updateQuery.addBindValue(imageInfo["colorChannel"  ]);
    if (imageInfo.contains("palette"       )) updateQuery.addBindValue(imageInfo["palette"       ]);

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

    QList<QVariant> results;
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

    QList<QVariant> results;
    QSqlQuery query;
    query.prepare("delete from images "
                  "where image_file = ?");
    query.addBindValue(localFile);
    query.exec();
}

QString WzDatabaseService::unpackDemoTiff(const QString &path)
{
    QDir dir;
    dir.mkpath(path);

    QFile demoTiff(":/demotiff/demo.tif");
    QFile demoMarkerTiff(":/demotiff/demo-marker.tif");
    QFile demoThumb(":/demotiff/demo-thumb.jpg");

    QString dateTimeStr = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    QString fnTiff = path + QString("/demo-%1.tif").arg(dateTimeStr);
    QString fnMarker = path + QString("/demo-marker-%1.tif").arg(dateTimeStr);
    QString fnThumb = path + QString("/demo-thumb-%1.jpg").arg(dateTimeStr);

    demoTiff.copy(fnTiff);
    demoMarkerTiff.copy(fnMarker);
    demoThumb.copy(fnThumb);

    QJsonObject imageInfoMap;

    imageInfoMap["imageFile"     ] = fnTiff;
    imageInfoMap["imageThumbFile"] = fnThumb;
    imageInfoMap["imageWhiteFile"] = fnMarker;
    imageInfoMap["imageSource"   ] = 1;
    imageInfoMap["createDate"    ] = "2020-03-27 00:00:00";
    imageInfoMap["updateDate"    ] = "2020-03-27 00:00:00";
    imageInfoMap["captureDate"   ] = "2020-03-27 00:00:00";
    imageInfoMap["exposureMs"    ] = 3000;
    imageInfoMap["sampleName"    ] = "demo";
    imageInfoMap["grayLow"       ] = 0;
    imageInfoMap["grayHigh"      ] = 12500;
    imageInfoMap["grayMax"       ] = 65007;
    imageInfoMap["grayMin"       ] = 74;
    imageInfoMap["groupId"       ] = 0;
    imageInfoMap["imageInvert"   ] = 0;
    imageInfoMap["openedLight"   ] = "white_up";
    imageInfoMap["grayLowMarker" ] = 0;
    imageInfoMap["grayHighMarker"] = 20000;
    imageInfoMap["colorChannel"  ] = 0;
    imageInfoMap["palette"       ] = "";
    imageInfoMap["loading"       ] = false;

    saveImage(imageInfoMap.toVariantMap());

    return fnTiff;
}

QStringList WzDatabaseService::unpackDemoTiffFluor(const QString &path)
{
    qInfo() << "***DatabaseService::unpackDemoTiffFluor***";

    QDir dir;
    dir.mkpath(path);

    auto unpack = [](const QString &path, const QString &suffix, const QString& light) -> QJsonObject {
        QFile demoTiff(QString(":/demotiff/demo%1.tif").arg(suffix));
        QFile demoMarkerTiff(QString(":/demotiff/demo-marker%1.tif").arg(suffix));
        QFile demoThumb(QString(":/demotiff/demo-thumb%1.jpg").arg(suffix));

        QString dateTimeStr = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        QString fnTiff = path + QString("/demo%1-%2.tif").arg(suffix, dateTimeStr);
        QString fnMarker = path + QString("/demo-marker%1-%2.tif").arg(suffix, dateTimeStr);
        QString fnThumb = path + QString("/demo-thumb%1-%2.jpg").arg(suffix, dateTimeStr);

        demoTiff.copy(fnTiff);
        demoMarkerTiff.copy(fnMarker);
        demoThumb.copy(fnThumb);

        QJsonObject imageInfoMap;

        imageInfoMap["imageFile"     ] = fnTiff;
        imageInfoMap["imageThumbFile"] = fnThumb;
        imageInfoMap["imageWhiteFile"] = fnMarker;
        imageInfoMap["imageSource"   ] = 1;
        imageInfoMap["createDate"    ] = "2020-03-27 00:00:00";
        imageInfoMap["updateDate"    ] = "2020-03-27 00:00:00";
        imageInfoMap["captureDate"   ] = "2020-03-27 00:00:00";
        imageInfoMap["exposureMs"    ] = 3000;
        imageInfoMap["sampleName"    ] = light;
        imageInfoMap["grayLow"       ] = 0;
        imageInfoMap["grayHigh"      ] = 12500;
        imageInfoMap["grayMax"       ] = 65007;
        imageInfoMap["grayMin"       ] = 74;
        imageInfoMap["groupId"       ] = 0;
        imageInfoMap["imageInvert"   ] = 0;
        imageInfoMap["openedLight"   ] = light;
        imageInfoMap["grayLowMarker" ] = 0;
        imageInfoMap["grayHighMarker"] = 20000;
        imageInfoMap["colorChannel"  ] = 0;
        imageInfoMap["palette"       ] = "";
        imageInfoMap["loading"       ] = false;

        return imageInfoMap;
    };

    QStringList fileNames;

    auto imageInfo1 = unpack(path, "", "red");
    saveImage(imageInfo1.toVariantMap());
    fileNames.append(imageInfo1["imageFile"].toString());

    auto imageInfo2 = unpack(path, "2", "green");
    saveImage(imageInfo2.toVariantMap());
    fileNames.append(imageInfo2["imageFile"].toString());

    auto imageInfo3 = unpack(path, "3", "blue");
    saveImage(imageInfo3.toVariantMap());
    fileNames.append(imageInfo3["imageFile"].toString());

    return fileNames;
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
}

bool WzDatabaseService::readBoolOption(const QString &optionName, const bool defaultValue)
{
    return 1 == readIntOption(optionName, defaultValue ? 1 : 0);
}

void WzDatabaseService::saveBoolOption(const QString &optionName, const bool value)
{
    saveIntOption(optionName, value ? 1 : 0);
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
    imageInfoMap["grayLow"       ] = query.value("gray_low"        ).toInt();
    imageInfoMap["grayHigh"      ] = query.value("gray_high"       ).toInt();
    imageInfoMap["grayMax"       ] = query.value("gray_max"        ).toInt();
    imageInfoMap["grayMin"       ] = query.value("gray_min"        ).toInt();
    imageInfoMap["groupId"       ] = query.value("group_id"        ).toInt();
    imageInfoMap["imageInvert"   ] = query.value("image_invert"    ).toInt();
    imageInfoMap["openedLight"   ] = query.value("opened_light"    );
    imageInfoMap["grayLowMarker" ] = query.value("gray_low_marker" ).toInt();
    imageInfoMap["grayHighMarker"] = query.value("gray_high_marker").toInt();
    imageInfoMap["colorChannel"  ] = query.value("color_channel"   ).toInt();
    imageInfoMap["palette"       ] = query.value("palette"         );
    imageInfoMap["loading"       ] = false;

    return imageInfoMap;
}

void WzDatabaseService::readIni()
{
    QString iniFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + kIniFileName;
    QSettings settings(iniFile, QSettings::IniFormat);
    QString language = settings.value("Normal/language", "").toString();
    if (language != "") {
        saveStrOption("languageName", language);
        settings.remove("Normal/language");
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
}

QString WzDatabaseService::readStrOptionAes(const QString &optionName, const QString &defaultValue)
{
    QString result = readStrOption(optionName, "");
    if (result == "")
        return defaultValue;
    return WzUtils::aesDecryptStr(s_aesKey, result);
}

void WzDatabaseService::saveStrOptionAes(const QString &optionName, const QString &value)
{
    QString encryptedValue = WzUtils::aesEncryptStr(s_aesKey, value);
    saveStrOption(optionName, encryptedValue);
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

void WzDatabaseService::showAllImageFiles(const QString &path)
{
    QDirIterator it(path, QStringList() << "*", QDir::Files, QDirIterator::Subdirectories);
    qDebug() << "All image files:";
    while (it.hasNext()) {
        auto s = it.next();
        qDebug() << s;
    }
}
