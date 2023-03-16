#include "ziputils.h"

ZipUtils::ZipUtils(QObject *parent): QObject{parent}
{
    qInfo() << Q_FUNC_INFO;
}

void ZipUtils::createZipFile(const QStringList fileNames, const QString &zipFileName)
{
    qInfo() << Q_FUNC_INFO;
    m_runningCount++;
    ZipThread* t = new ZipThread(this);
    connect(t, &ZipThread::finished, this, &ZipUtils::threadFinished);
    t->setFileNames(fileNames);
    t->setZipFileName(zipFileName);
    t->start();
}

int ZipUtils::runningCount() const
{
    return m_runningCount;
}

void ZipUtils::threadFinished()
{
    qInfo() << Q_FUNC_INFO;
    ZipThread* t = qobject_cast<ZipThread*>(sender());
    if (t) {
        if (t->error() != SUCCESS) {
            emit error(t->error(), t->zipFileName());
        } else {
            emit finished();
        }
        t->deleteLater();
    }
    m_runningCount--;
}

int ZipThread::createZipFile(const QStringList fileNames, const QString &zipFileName)
{
    qInfo() << Q_FUNC_INFO;
    qInfo() << "\tfileNames:" << fileNames;
    qInfo() << "\tzipFileName:" << zipFileName;

    QFileInfo fi7zip(QCoreApplication::applicationDirPath(), "7za.exe");
    if (!fi7zip.exists()) {
        qWarning() << fi7zip.absoluteFilePath();
        return NOT_FOUND_7ZIP;
    }


    QProcess process7zip;

    process7zip.connect(&process7zip, &QProcess::readyReadStandardOutput, [&process7zip]{
        QByteArray ba = process7zip.readAllStandardOutput();
        foreach(auto s, QString::fromLocal8Bit(ba).split("\r\n")) {
            qInfo() << s;
        }
    });

    QStringList args;
    args << "a"
         << "-mx=9"
         << "-sdel"
         << zipFileName
         << fileNames;
    process7zip.start(fi7zip.absoluteFilePath(), args);
    qInfo() << "\t" << process7zip.arguments();
    auto is7zipTimeout = !process7zip.waitForFinished();

    QFileInfo fiFirstFile(fileNames[0]);
    QDir tempDir(fiFirstFile.absoluteFilePath());
    tempDir.removeRecursively();

    if (is7zipTimeout)
        return TIMEOUT_7ZIP;

    return SUCCESS;
}

ZipThread::ZipThread(QObject *parent): QThread{parent}
{
    qInfo() << Q_FUNC_INFO;
}

const QStringList &ZipThread::fileNames() const
{
    return m_fileNames;
}

void ZipThread::setFileNames(const QStringList &newFileNames)
{
    m_fileNames = newFileNames;
}

const QString &ZipThread::zipFileName() const
{
    return m_zipFileName;
}

void ZipThread::setZipFileName(const QString &newZipFileName)
{
    m_zipFileName = newZipFileName;
}

void ZipThread::run()
{
    qInfo() << Q_FUNC_INFO;
    m_error = createZipFile(m_fileNames, m_zipFileName);
    qInfo() << "\terror:" << m_error;
}

int ZipThread::error() const
{
    return m_error;
}

