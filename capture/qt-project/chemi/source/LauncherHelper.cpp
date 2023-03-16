#include "../include/LauncherHelper.h"

LauncherHelper::LauncherHelper(QObject *parent) : QObject(parent)
{

}

void LauncherHelper::started()
{
    QCommandLineParser parser;
    parser.addOption({"startedEvent", "", "startedEvent"});
    parser.parse(qApp->arguments());
    QString startedEventName = parser.value("startedEvent");
    if (startedEventName != "") {
#ifdef Q_OS_WIN        
        HANDLE h = OpenEventA(EVENT_ALL_ACCESS,
                              false,
                              startedEventName.toLocal8Bit().data());
        if (h) {
            SetEvent(h);
            CloseHandle(h);
        } else {
            qWarning("OpenEvent(started) failure");
        }
#endif
    } else {
        qWarning("startedEventName == null");
    }
}

void LauncherHelper::exiting()
{
    QCommandLineParser parser;
    parser.addOption({"exitingEvent", "", "exitingEvent"});
    parser.parse(qApp->arguments());
    QString exitingEventName = parser.value("exitingEvent");

    if (exitingEventName != "") {
#ifdef Q_OS_WIN
        HANDLE h = OpenEventA(EVENT_ALL_ACCESS,
                              false,
                              exitingEventName.toLocal8Bit().data());
        if (h) {
            SetEvent(h);
            CloseHandle(h);
        } else {
            qWarning("OpenEvent(exiting) failure");
        }
#endif
    } else {
        qWarning("exitingEventName == null");
    }
}
