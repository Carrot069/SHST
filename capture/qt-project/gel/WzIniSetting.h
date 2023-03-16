#ifndef WZINISETTING_H
#define WZINISETTING_H

#include <QObject>
#include <QApplication>
#include <QStandardPaths>
#include <QSettings>

#include "WzGlobalConst.h"

class WzIniSetting: public QObject
{
    Q_OBJECT
public:
    explicit WzIniSetting(QObject *parent = nullptr);

    Q_INVOKABLE bool readBool(const QString &secName, const QString &keyName,
                              const bool &defaultValue = false);
    Q_INVOKABLE void saveBool(const QString &secName, const QString &keyName,
                              const bool &value);
private:
    QString m_iniFileName;
    QString getIniFileName() const;
};

#endif // WZINISETTING_H
