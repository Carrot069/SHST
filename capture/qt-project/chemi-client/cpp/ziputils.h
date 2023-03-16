#ifndef ZIPUTILS_H
#define ZIPUTILS_H

#include <QObject>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QRandomGenerator>
#include <QDebug>
#include <QTextStream>
#include <QProcess>
#include <QThread>
#include <QCoreApplication>

const int SUCCESS = 0;
const int NOT_FOUND_7ZIP = -1;
const int FILE_NOT_OPEN = -2;
const int TIMEOUT_7ZIP = -3;

class ZipThread: public QThread
{
    Q_OBJECT
public:
    explicit ZipThread(QObject *parent = nullptr);

    const QStringList &fileNames() const;
    void setFileNames(const QStringList &newFileNames);

    const QString &zipFileName() const;
    void setZipFileName(const QString &newZipFileName);

    int error() const;

protected:
    void run() override;

private:
    QStringList m_fileNames;
    QString m_zipFileName;
    int m_error = SUCCESS;

    int createZipFile(const QStringList fileNames, const QString& zipFileName);
};

class ZipUtils: public QObject
{
    Q_OBJECT
public:

    explicit ZipUtils(QObject *parent = nullptr);

    void createZipFile(const QStringList fileNames, const QString& zipFileName);

    int runningCount() const;

private:
    int m_runningCount = 0;

private slots:
    void threadFinished();

signals:
    void finished();
    void error(const int error, const QString& zipFileName);
};

#endif // ZIPUTILS_H
