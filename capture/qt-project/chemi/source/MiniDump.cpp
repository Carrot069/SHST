#include "MiniDump.h"

#ifdef QT_DEBUG
#ifdef Q_OS_WIN
void showCrashWindow() {
    auto app = QApplication::instance();
    QString crashFileName = app->applicationDirPath() + "/crash.exe";
    QString mainExeName = QFileInfo(app->applicationFilePath()).fileName();
    QStringList args;
    // 后面将"通过命令行参数传递界面语言的选项"改为"从ini文件中读取界面语言的选项"
    // 主程序在切换语言后将其写入到ini文件, 如此修改是防止主程序内读取界面语言选项时出错,
    // 进而导致没有运行 crash.exe
    args << mainExeName << "zh";
    QProcess::startDetached(crashFileName, args);
}

LONG WINAPI ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) +
        "/" + kOrganizationName + "/" + kApplicationName;
    QDir desktopDir(desktopPath);
    if (!desktopDir.exists())
        desktopDir.mkpath(".");
    QString dumpFileName = QDateTime::currentDateTime().toString("yyyy-MM-dd hh.mm.ss.zzz") + ".dmp";
    QString fullFileName = QFileInfo(desktopDir, dumpFileName).absoluteFilePath();
    wchar_t fnBuffer[65535] = {0};
    fullFileName.toWCharArray(fnBuffer);

    HANDLE hDumpFile = CreateFile(fnBuffer,
                                  GENERIC_WRITE,
                                  0,
                                  nullptr,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  nullptr);
    if (hDumpFile != INVALID_HANDLE_VALUE) {
        //Dump信息
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pException;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;
        //写入Dump文件内容
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
        CloseHandle(hDumpFile);
    }

    showCrashWindow();

    return EXCEPTION_EXECUTE_HANDLER;
}

void InstallMiniDump() {
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
}
#endif
#endif
