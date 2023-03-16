#ifndef WZHAIERCONF_H
#define WZHAIERCONF_H

#include <iostream>
#include <fstream>

#include <QObject>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>
#include <QSettings>

#include "aes/aes.hpp"
#include "WzGlobalConst.h"
#include "WzUtils.h"

class WzHaierConf : public QObject
{
    Q_OBJECT
public:
    explicit WzHaierConf(QObject *parent = nullptr);

    Q_INVOKABLE bool isExists() const;
    Q_INVOKABLE bool readConfig();

    Q_PROPERTY(QString clientId MEMBER m_clientId);
    Q_PROPERTY(QString clientSecret MEMBER m_clientSecret);
    Q_PROPERTY(QString username MEMBER m_username);
    Q_PROPERTY(QString password MEMBER m_password);
    Q_PROPERTY(QString devCode MEMBER m_devCode);
    Q_PROPERTY(QString devName MEMBER m_devName);
    Q_PROPERTY(QString devModel MEMBER m_devModel);
    Q_PROPERTY(QString office MEMBER m_office);
    Q_PROPERTY(QString serverUrl MEMBER m_serverUrl);
    Q_PROPERTY(QString tokenUrl MEMBER m_tokenUrl);
    Q_PROPERTY(QString heartUrl MEMBER m_heartUrl);
    Q_PROPERTY(QString saveDeviceUrl MEMBER m_saveDeviceUrl);
    Q_PROPERTY(QString updateDeviceUrl MEMBER m_updateDeviceUrl);
    Q_PROPERTY(QString saveDeviceDataUrl MEMBER m_saveDeviceDataUrl);

private:
    QString getConfFileName() const;
    void aesDecrypt(aes256_key key, QString inFileName, QByteArray &content) const;

    QString m_clientId;
    QString m_clientSecret;
    QString m_username;
    QString m_password;
    QString m_devCode;
    QString m_devName;
    QString m_devModel;
    QString m_office;
    QString m_serverUrl;
    QString m_tokenUrl;
    QString m_heartUrl;
    QString m_saveDeviceUrl;
    QString m_updateDeviceUrl;
    QString m_saveDeviceDataUrl;

signals:

};

#endif // WZHAIERCONF_H
