#ifndef SHSTUSBDISK_H
#define SHSTUSBDISK_H

#include <QObject>
#include <QStringList>
#include <QTemporaryFile>
#include <QProcess>
#include <QDebug>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif

class ShstUsbDisk : public QObject
{
    Q_OBJECT
public:
    explicit ShstUsbDisk(QObject *parent = nullptr);

    // 获取可用U盘数量
    int getCount(QStringList *drives = nullptr);

    // 把文件复制到所有可用的U盘中
    // destFiles 中的文件名将会被追加到U盘盘符后面, 如
    // 在Windows下面U盘的盘符是 "D:", 文件名是 "/a/b/c.txt",
    // 则文件将被复制为 "D:/a/b/c.txt"
    // 函数返回成功复制的文件数量, 如果失败则可通过 errorString() 获取错误信息
    int copyFilesToUsbDisks(const QStringList &sourceFiles, const QStringList &destFiles);

    QString errorString() const;

private:
    QString m_errorString;
signals:

};

#endif // SHSTUSBDISK_H
