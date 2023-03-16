#include "WzAppSingleton.h"

bool WzAppSingleton::appInstanceAlready() const
{
    return m_appInstanceAlready;
}

WzAppSingleton::WzAppSingleton(QObject *parent) : QObject(parent)
{
    qInfo() << "AppSingleton";
    QString mutexName = kApplicationName;
#ifdef Q_OS_WIN
    m_mutexHandle = CreateMutexW(nullptr, false, mutexName.toStdWString().c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        m_appInstanceAlready = true;
    }
#endif
}

WzAppSingleton::~WzAppSingleton()
{
    qInfo() << "~AppSingleton";
#ifdef Q_OS_WIN
    CloseHandle(m_mutexHandle);
#endif
}
