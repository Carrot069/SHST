#include "StartProcess.h"

StartProcess::StartProcess(QObject *parent) : QObject(parent)
{

}

void StartProcess::setExeName(const QString &exeName)
{
    m_exeName = exeName;
}

void StartProcess::startProcess()
{
#ifdef Q_OS_WIN
    QDir dir(qApp->applicationDirPath());
    QFileInfo fi(dir, m_exeName);
    if (!fi.exists()) {
        qInfo() << "File not found:" << fi.absoluteFilePath();
    } else {
        QProcess::startDetached(fi.absoluteFilePath());
    }
#endif
}
