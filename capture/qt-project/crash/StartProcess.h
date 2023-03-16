#ifndef STARTPROCESS_H
#define STARTPROCESS_H

#include <QObject>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QProcess>

class StartProcess : public QObject
{
    Q_OBJECT
public:
    explicit StartProcess(QObject *parent = nullptr);

    void setExeName(const QString &exeName);
    Q_INVOKABLE void startProcess();
signals:

private:
    QString m_exeName;
};

#endif // STARTPROCESS_H
