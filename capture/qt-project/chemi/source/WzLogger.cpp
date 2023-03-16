#include "WzLogger.h"

WzLogger::WzLogger(QObject *parent) : QObject(parent)
{

}

bool WzLogger::startDbgView()
{
#ifdef Q_OS_WIN
    if (dbgViewExists())
        return true;

    const int nBufSize = 512;
    TCHAR chBuf[nBufSize];
    ZeroMemory(chBuf, nBufSize);

    // 获取当前执行文件的路径
    QFileInfo appFullName;
    if (GetModuleFileName(NULL, chBuf, nBufSize)) {
        appFullName = QFileInfo(QString::fromWCharArray(chBuf));
    } else {
        return false;
    }

    QFileInfo dbgViewFileInfo(QDir(appFullName.absolutePath()), dbgViewFileName);
    if (dbgViewFileInfo.exists()) {
        QDir logDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        logDir.mkpath(kOrganizationName);
        if (!logDir.cd(kOrganizationName))
            return false;
        logDir.mkpath(kApplicationName);
        if (!logDir.cd(kApplicationName))
            return false;
        QString logFileName = logDir.absolutePath() + "/" + kApplicationName + ".log";

        QProcess* p = new QProcess(this);
        QString arguments = "/accepteula /l \"" + logFileName + "\" /t /p /om /n";
        p->setNativeArguments(arguments);
        p->start("\"" + dbgViewFileInfo.absoluteFilePath() + "\"");
        return true;
    } else {
        return false;
    }
#else
    return false;
#endif
}

void WzLogger::stopDbgView()
{

}

#ifdef Q_OS_WIN
bool WzLogger::dbgViewExists()
{
#ifdef Q_OS_WIN
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hProcessSnap == INVALID_HANDLE_VALUE)
    {
        printError("CreateToolhelp32Snapshot (of processes)");
        return false;
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if(!Process32First(hProcessSnap, &pe32))
    {
        printError("Process32First"); // show cause of failure
        CloseHandle(hProcessSnap);          // clean the snapshot object
        return false;
    }

    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do
    {
        if (dbgViewFileName == QString::fromWCharArray(pe32.szExeFile).toLower()) {
            QString fullName;
            listProcessModules(pe32.th32ProcessID, dbgViewFileName, fullName);
            if (fullName != "") {
                qDebug() << fullName;
                CloseHandle(hProcessSnap);
                return true;
            }
        }

    } while(Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);

    return false;
#else
    return false;
#endif
}

bool WzLogger::listProcessModules(DWORD dwPID, const QString &modName, QString &fullName)
{
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    MODULEENTRY32 me32;

    // Take a snapshot of all modules in the specified process.
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
    if(hModuleSnap == INVALID_HANDLE_VALUE)
    {
        printError("CreateToolhelp32Snapshot (of modules)");
        return false;
    }

    // Set the size of the structure before using it.
    me32.dwSize = sizeof(MODULEENTRY32);

    // Retrieve information about the first module,
    // and exit if unsuccessful
    if(!Module32First(hModuleSnap, &me32))
    {
        printError("Module32First");  // show cause of failure
        CloseHandle(hModuleSnap);           // clean the snapshot object
        return false;
    }

    // Now walk the module list of the process,
    // and display information about each module
    do
    {
        if (QString::fromWCharArray(me32.szModule).toLower() == modName) {
            fullName = QString::fromWCharArray(me32.szExePath);
            break;
        }
    } while(Module32Next(hModuleSnap, &me32));

    CloseHandle(hModuleSnap);

    return true;
}

void WzLogger::printError(const QString &msg)
{
    DWORD eNum;
    TCHAR sysMsg[256];
    TCHAR* p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, eNum,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  sysMsg, 256, NULL);

    // Trim the end of the line and terminate it with a null
    p = sysMsg;
    while((*p > 31) || (*p == 9))
        ++p;
    do {*p-- = 0;} while((p >= sysMsg) &&
             ((*p == '.' ) || (*p < 33)));

    // Display the message
    qWarning() << QString("WARNING: %s failed with error %d (%s)").arg(msg).arg(eNum).arg(sysMsg);
}
#endif
