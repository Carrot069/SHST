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
    newFields["gray_low_marker"] = "int";       // low gray of marker
    newFields["gray_high_marker"] = "int";      // high gray of marker
    newFields["color_channel"] = "int";         // 颜色通道, 0 代表未叠加, 1/2/3 依次为 R/G/B
    newFields["palette"] = "text";              // 调色板
    newFields["is_deleted"] = "int";            // 是否删除
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
    appendSql(imageInfo, "grayLowMarker"  , "gray_low_marker" , updateSql);
    appendSql(imageInfo, "grayHighMarker" , "gray_high_marker", updateSql);
    appendSql(imageInfo, "colorChannel"   , "color_channel"   , updateSql);
    appendSql(imageInfo, "isDeleted"      , "is_deleted"      , updateSql);

    if ("" == updateSql)
        return false;

    QSqlQuery updateQuery(db);
    updateSql = "update images set " + updateSql + " where image_file = ?";
    updateQuery.prepare(updateSql);
    qDebug() << updateSql;

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
    if (imageInfo.contains("isDeleted"     )) updateQuery.addBindValue(imageInfo["isDeleted"     ]);

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
    query.prepare("select * from images where is_deleted is null or is_deleted <> 1 order by update_date");
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

    //QList<QVariant> results;
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

    //QList<QVariant> results;
    QSqlQuery query;
    query.prepare("update images "
                  "set is_deleted = 1 "
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
    imageInfoMap["isDeleted"     ] = query.value("is_deleted"      ).toInt();

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

QString WzDatabaseService::getAdvSetFullFile() const
{
#ifdef Second_Chemi
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + ADV_SET_Send_FILE;

#else
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + ADV_SET_FILE;
#endif
}

bool WzDatabaseService::checkAdvSetIni() const
{
    return QFileInfo::exists(getAdvSetFullFile());
}

bool WzDatabaseService::deleteAdvSetIni() const
{
    return QFile::remove(getAdvSetFullFile());
}

void WzDatabaseService::saveAdminSettingToDisk(QVariantMap params)
{
    QSettings settings(getAdvSetFullFile(), QSettings::IniFormat);

    auto chemi = params["chemi"].toMap();
    settings.setValue("chemi/light", chemi["light"].toInt());
    settings.setValue("chemi/filter", chemi["filter"].toInt());
    settings.setValue("chemi/aperture", chemi["aperture"].toInt());
    settings.setValue("chemi/binning", chemi["binning"].toInt());
    settings.setValue("chemi/exposureMs", chemi["exposureMs"].toInt());
    settings.setValue("chemi/previewExposureMs", chemi["previewExposureMs"].toInt());

    auto rna = params["rna"].toMap();
    settings.setValue("rna/light", rna["light"].toInt());
    settings.setValue("rna/filter", rna["filter"].toInt());
    settings.setValue("rna/aperture", rna["aperture"].toInt());
    settings.setValue("rna/binning", rna["binning"].toInt());
    settings.setValue("rna/exposureMs", rna["exposureMs"].toInt());

    auto protein = params["protein"].toMap();
    settings.setValue("protein/light", protein["light"].toInt());
    settings.setValue("protein/filter", protein["filter"].toInt());
    settings.setValue("protein/aperture", protein["aperture"].toInt());
    settings.setValue("protein/binning", protein["binning"].toInt());
    settings.setValue("protein/exposureMs", protein["exposureMs"].toInt());

    auto red = params["red"].toMap();
    settings.setValue("red/light", red["light"].toInt());
    settings.setValue("red/filter", red["filter"].toInt());
    settings.setValue("red/aperture", red["aperture"].toInt());
    settings.setValue("red/binning", red["binning"].toInt());
    settings.setValue("red/exposureMs", red["exposureMs"].toInt());

    auto green = params["green"].toMap();
    settings.setValue("green/light", green["light"].toInt());
    settings.setValue("green/filter", green["filter"].toInt());
    settings.setValue("green/aperture", green["aperture"].toInt());
    settings.setValue("green/binning", green["binning"].toInt());
    settings.setValue("green/exposureMs", green["exposureMs"].toInt());

    auto blue = params["blue"].toMap();
    settings.setValue("blue/light", blue["light"].toInt());
    settings.setValue("blue/filter", blue["filter"].toInt());
    settings.setValue("blue/aperture", blue["aperture"].toInt());
    settings.setValue("blue/binning", blue["binning"].toInt());
    settings.setValue("blue/exposureMs", green["exposureMs"].toInt());

    settings.setValue("common/binningVisible", params["binningVisible"].toBool());
    settings.setValue("common/grayAccumulateAddExposure", params["grayAccumulateAddExposure"].toBool());
    settings.setValue("common/repeatSetFilter", params["repeatSetFilter"].toBool());
    settings.setValue("common/pvcamSlowPreview", params["pvcamSlowPreview"].toBool());
    settings.setValue("common/isFilterWheel8", params["isFilterWheel8"].toBool());
    settings.setValue("common/customFilterWheel", params["customFilterWheel"].toBool());
    settings.setValue("common/bluePenetrateAlone", params["bluePenetrateAlone"].toBool());
    settings.setValue("common/hideUvPenetrate", params["hideUvPenetrate"].toBool());
    settings.setValue("common/hideUvPenetrateForce", params["hideUvPenetrateForce"].toBool());

    settings.setValue("common/fluorPreviewExposureMs", params["fluorPreviewExposureMs"].toInt());

    QVariantList filterOptions = params["filters"].toList();
    for (int i = 0; i < filterOptions.length(); i++) {
        if (i >= 8)
            break;

        QVariantMap filter = filterOptions.at(i).toMap();
        settings.setValue(QString("filters/color%1").arg(i), filter["color"].toString());
        settings.setValue(QString("filters/wavelength%1").arg(i), filter["wavelength"].toString());
    }

    auto layerPWMCount = params["layerPWMCount"].toList();
    if (layerPWMCount.count() > 0) {
        QString layerPWMCountStr = QString::number(layerPWMCount[0].toInt());
        for (int i = 1; i < layerPWMCount.count(); i++) {
            layerPWMCountStr += QString(",%1").arg(layerPWMCount[i].toInt());
        }
        settings.setValue("common/layerPWMCount", layerPWMCountStr);
    }

    settings.setValue("common/RY3ID", params["RY3ID"].toString());
}

QVariantMap WzDatabaseService::readAdminSettingFromDisk() const
{
    QSettings settings(getAdvSetFullFile(), QSettings::IniFormat);
    QVariantMap params;

    QVariantMap chemi;
    chemi["light"            ] = settings.value("chemi/light").toInt();
    chemi["filter"           ] = settings.value("chemi/filter").toInt();
    chemi["aperture"         ] = settings.value("chemi/aperture").toInt();
    chemi["binning"          ] = settings.value("chemi/binning").toInt();
    chemi["exposureMs"       ] = settings.value("chemi/exposureMs").toInt();
    chemi["previewExposureMs"] = settings.value("chemi/previewExposureMs").toInt();
    params["chemi"] = chemi;

    QVariantMap rna;

    rna["light"     ] = settings.value("rna/light").toInt();
    rna["filter"    ] = settings.value("rna/filter").toInt();
    rna["aperture"  ] = settings.value("rna/aperture").toInt();
    rna["binning"   ] = settings.value("rna/binning").toInt();
    rna["exposureMs"] = settings.value("rna/exposureMs").toInt();
    params["rna"] = rna;

    QVariantMap protein;
    protein["light"     ] = settings.value("protein/light").toInt();
    protein["filter"    ] = settings.value("protein/filter").toInt();
    protein["aperture"  ] = settings.value("protein/aperture").toInt();
    protein["binning"   ] = settings.value("protein/binning").toInt();
    protein["exposureMs"] = settings.value("protein/exposureMs").toInt();
    params["protein"] = protein;

    QVariantMap red = params["red"].toMap();
    red["light"     ] = settings.value("red/light").toInt();
    red["filter"    ] = settings.value("red/filter").toInt();
    red["aperture"  ] = settings.value("red/aperture").toInt();
    red["binning"   ] = settings.value("red/binning").toInt();
    red["exposureMs"] = settings.value("red/exposureMs").toInt();
    params["red"] = red;

    QVariantMap green;
    green["light"     ] = settings.value("green/light").toInt();
    green["filter"    ] = settings.value("green/filter").toInt();
    green["aperture"  ] = settings.value("green/aperture").toInt();
    green["binning"   ] = settings.value("green/binning").toInt();
    green["exposureMs"] = settings.value("green/exposureMs").toInt();
    params["green"] = green;

    QVariantMap blue;
    blue["light"     ] = settings.value("blue/light").toInt();
    blue["filter"    ] = settings.value("blue/filter").toInt();
    blue["aperture"  ] = settings.value("blue/aperture").toInt();
    blue["binning"   ] = settings.value("blue/binning").toInt();
    blue["exposureMs"] = settings.value("blue/exposureMs").toInt();
    params["blue"] = blue;

    params["binningVisible"           ] = settings.value("common/binningVisible").toBool();
    params["grayAccumulateAddExposure"] = settings.value("common/grayAccumulateAddExposure").toBool();
    params["repeatSetFilter"          ] = settings.value("common/repeatSetFilter").toBool();
    params["pvcamSlowPreview"         ] = settings.value("common/pvcamSlowPreview").toBool();
    params["isFilterWheel8"           ] = settings.value("common/isFilterWheel8").toBool();
    params["customFilterWheel"        ] = settings.value("common/customFilterWheel").toBool();
    params["bluePenetrateAlone"       ] = settings.value("common/bluePenetrateAlone").toBool();

    params["hideUvPenetrate"          ] = settings.value("common/hideUvPenetrate").toBool();
    params["hideUvPenetrateForce"     ] = settings.value("common/hideUvPenetrateForce").toBool();
    params["fluorPreviewExposureMs"   ] = settings.value("common/fluorPreviewExposureMs").toInt();

    QVariantList filterOptions;
    for (int i = 0; i < 8; i++) {
        QVariantMap filter;
        filter["color"] = settings.value(QString("filters/color%1").arg(i)).toString();
        filter["wavelength"] = settings.value(QString("filters/wavelength%1").arg(i)).toString();
        filterOptions.append(filter);
    }
    params["filters"] = filterOptions;

    QStringList layerPWMCountList = settings.value("common/layerPWMCount").toString().split(",");
    QVariantList layerPWMCount;
    for (int i = 0; i < layerPWMCountList.count(); i++) {
        layerPWMCount.append(layerPWMCountList[i].toInt());
    }
    params["layerPWMCount"] = layerPWMCount;

    params["RY3ID"] = settings.value("common/RY3ID").toString();

    return params;
}

void WzDatabaseService::saveValueToDisk(const QString &keyName, const QVariant value)
{
    QSettings settings(getAdvSetFullFile(), QSettings::IniFormat);
    settings.setValue(keyName, value);
}

QVariant WzDatabaseService::readValueFromDisk(const QString &keyName, const QVariant defaultValue)
{
    QSettings settings(getAdvSetFullFile(), QSettings::IniFormat);
    return settings.value(keyName, defaultValue);
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
    query.prepare("select option_int from options where option_name = ?"); //查询sql语句
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

bool WzDatabaseService::existsStrOption(const QString &optionName)
{
    QSqlQuery query;
    query.prepare("select option_text from options where option_name = ?");
    query.addBindValue(optionName);
    query.exec();
    if (query.next()) {
        return true;
    } else {
        return false;
    }
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
    int checkAdvSetIniCount = 2;
    QVariantMap params;
#endif
//#define TEST_ERROR
#ifdef TEST_ERROR
    params = WzSetting2::readAdminSettingDefault();
    params["errorCode"] = SHST_RY3_NOT_FOUND;
    return params;
#endif
#ifdef HARDLOCK
    while(checkAdvSetIniCount > 0) {
        bool isExists = checkAdvSetIni();
        checkAdvSetIniCount--;
        if (isExists) {
            params = readAdminSettingFromDisk();
            QString curRy3Sn;
            int ret = WzRenderThread::getFirstSn(curRy3Sn);
            if (ret != SHST_SUCCESS) {
                params["errorCode"] = ret;
                return params;
            }
            if (curRy3Sn != params["RY3ID"].toString()) {
                if (!deleteAdvSetIni()) {
                    params["errorCode"] = SHST_DELETE_ADVSETINI_FAILED;
                    return params;
                };
                continue;
            } else {
                break;
            }
        } else {
            int retryCount = 3;
            while (retryCount > 0) {
                retryCount--;
                int retCode = WzSetting2::readAdminSetting2(params);
                if (retCode != SHST_SUCCESS) {
                    params = WzSetting2::readAdminSettingDefault();
                    params["errorCode"] = retCode;
                    return params;
                } else {
                    break;
                }
            }
            saveAdminSettingToDisk(params);
            break;
        }
    }
#endif
#ifdef HARDLOCK
    #if 0
    QVariantMap params = WzSetting2::readAdminSetting();
    #endif
#else
    QString jsonStr = readStrOption("admin_setting", "{}");
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
    QVariantMap params = jsonDoc.toVariant().toMap();
#endif
    if (params.count() == 0) {
        AdminSettingStruct buf;
        params = WzSetting2::buf2Map(buf);
    }
    if (!params.contains("fluorPreviewExposureMs"))
        params["fluorPreviewExposureMs"] = 10;
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
