#include "WzIniSetting.h"

WzIniSetting::WzIniSetting(QObject *parent)
    : QObject(parent),
    m_iniFileName(getIniFileName())
{

}

bool WzIniSetting::readBool(const QString &secName, const QString &keyName, const bool &defaultValue)
{
    QSettings settings(m_iniFileName, QSettings::IniFormat);
    return settings.value(secName + "/" + keyName, defaultValue).toBool();
}

void WzIniSetting::saveBool(const QString &secName, const QString &keyName, const bool &value)
{
    QSettings settings(m_iniFileName, QSettings::IniFormat);
    settings.setValue(secName + "/" + keyName, value);
}

QString WzIniSetting::getIniFileName() const
{
    QString result = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!result.contains(kOrganizationName))
        result += "/" + QString(kOrganizationName);
    if (!result.contains(kApplicationName))
        result += "/" + QString(kApplicationName);
    result += "/" + QString(kIniFileName);
    return result;
}
