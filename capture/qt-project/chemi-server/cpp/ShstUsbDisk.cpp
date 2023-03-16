#include "ShstUsbDisk.h"

ShstUsbDisk::ShstUsbDisk(QObject *parent) : QObject(parent)
{

}

int ShstUsbDisk::getCount(QStringList *drives)
{
    QStringList scripts =
        {"Set objWMIService = GetObject(\"winmgmts:{impersonationLevel=impersonate}!\\\\.\\root\\cimv2\")",
         "Set colDisks = objWMIService.ExecQuery (\"SELECT * FROM Win32_LogicalDisk WHERE DriveType = 2\")",
         "For Each objDisk in colDisks",
         "  If objDisk.FreeSpace > 0 Then",
         "    Wscript.Echo \"Drive\" & objDisk.DeviceID",
         "  End If",
         "Next"};
    QTemporaryFile *scriptFile = new QTemporaryFile();
    scriptFile->setAutoRemove(false);
    if (scriptFile->open()) {
        scriptFile->write(scripts.join("\r\n").toUtf8());
        scriptFile->close();
        QString scriptFileName = scriptFile->fileName();
        delete scriptFile;
        QProcess cscript;
        auto isProcessExit = false;
        cscript.connect(&cscript, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [&isProcessExit](int exitCode, QProcess::ExitStatus exitStatus){
            Q_UNUSED(exitCode)
            Q_UNUSED(exitStatus)
            isProcessExit = true;
        });
        auto driveList = QStringList();
        cscript.connect(&cscript, &QProcess::readyReadStandardOutput, [&cscript, &driveList]{
            QByteArray ba = cscript.readAllStandardOutput();
            foreach(auto s, QString::fromLocal8Bit(ba).split("\r\n")) {
                qDebug() << s;
                if (s.startsWith("Drive")) {
                    driveList.append(s.mid(5, 2));
                }
            }
        });
        cscript.start("cscript.exe", {"//E:vbscript", scriptFileName});
        while (!isProcessExit) {
            QCoreApplication::processEvents();
        }
        if (drives) {
            (*drives) << driveList;
        }
        return driveList.count();
    } else {
        if (scriptFile->exists())
            scriptFile->remove();
        delete scriptFile;
        m_errorString = "The script file cannot open";
        return -1;
    }
}

int ShstUsbDisk::copyFilesToUsbDisks(const QStringList &sourceFiles, const QStringList &destFiles)
{
    QStringList drives;
    if (getCount(&drives) == 0)
        return - 1;
    int count = 0;
    foreach(QString drive, drives) {
        for (int i = 0; i < sourceFiles.count(); i++) {
            if (QFile::exists(sourceFiles[i])) {
                QFileInfo destFile(drive + destFiles[i]);
                QDir destDir(destFile.absolutePath());
                if (!destDir.exists()) {
                    destDir.mkpath(".");
                }
                qInfo() << "Copy file:"
                        << sourceFiles[i]
                        << " -> "
                        << destFile.absoluteFilePath();
                if (QFile::copy(sourceFiles[i], destFile.absoluteFilePath()))
                    count++;
            }
        }
    }
    return count;
}

QString ShstUsbDisk::errorString() const
{
    return m_errorString;
}
