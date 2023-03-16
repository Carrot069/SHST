#ifndef WZSHAREFILE_H
#define WZSHAREFILE_H

#include <QObject>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QFileInfo>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QTemporaryDir>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidIntent>
#include <QAndroidJniEnvironment>
#endif
#include <WzImageService.h>
#include <WzImageView.h>
#include <cpp/ShstTcpIo.h>
#include <cpp/ShstTcpFileIo.h>
#include <cpp/ShstFileToUsbDiskClient.h>
#ifdef Q_OS_WIN
#include <cpp/ziputils.h>
#endif

class FileToUsbDiskThread : public QThread
{
    Q_OBJECT
public:
    explicit FileToUsbDiskThread(QObject *parent = nullptr);
    ~FileToUsbDiskThread() override;
    void setOptions(const QJsonObject& options);
    void sendFiles(const QStringList &remoteFiles, const QStringList &localFiles);
    void stop();

protected:
    void run() override;

private:
    bool m_isStop = false;
    QMutex m_sendFilesMutex;
    QWaitCondition m_sendFilesWaitCondition;
    QJsonObject m_options;
    QStringList m_remoteFiles;
    QStringList m_localFiles;
signals:
    void sendFilesFinished();
};

class WzShareFile : public QObject
{
    Q_OBJECT
private:
    static WzShareFile *m_instance;

    FileToUsbDiskThread* m_fileToUsbDiskThread;
    WzImageService* m_imageService = nullptr;
    WzImageView* m_imageView = nullptr;
    QJsonObject m_options;

    void makeImageService();
    QString getUsbDiskBasePath();
    void filesToUsbDisk();

#ifdef Q_OS_WIN
    ZipUtils *m_zipUtils = nullptr;
#endif

public:
    explicit WzShareFile(QObject *parent = nullptr);
    ~WzShareFile() override;

    static WzShareFile *instance();

    Q_INVOKABLE void shareFile(const QString& fileName, const QJsonObject &options = QJsonObject());
    Q_INVOKABLE void shareFiles(const QJsonArray &fileNames, const QJsonObject &options = QJsonObject());

    Q_INVOKABLE static QString getShareDir();
    Q_INVOKABLE static QString getShareFileName(const QString &fileName, const QString &imageFormat);
    Q_INVOKABLE static QString getZipFileName(const QJsonObject &images);
    Q_INVOKABLE static void clearHistoryFiles();

    Q_INVOKABLE QJsonArray makeSharedImages(const QJsonObject &imageInfo, const QJsonObject &options, const QString &path);

    void zipFinishedFromJava();
signals:
    void finished();
    void zipStart();
    void zipProgress(int done, int total);
    void zipFinished();
    void toUsbDisk();
    void toUsbDiskFinished();

private slots:
    void fileToUsbDiskFinished();
};

#endif // WZSHAREFILE_H
