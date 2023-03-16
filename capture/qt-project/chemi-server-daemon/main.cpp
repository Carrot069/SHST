#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QProcess>
#include <QFileInfo>

#include <windows.h>
#include <tlhelp32.h>// for CreateToolhelp32Snapshot

bool GetProcessList(QStringList &processList)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        qDebug() << "CreateToolhelp32Snapshot (of processes)";
        return false;
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32)) {
        qDebug() << "Process32First"; // show cause of failure
        CloseHandle(hProcessSnap);   // clean the snapshot object
        return false;
    }

    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do {
        processList << QString::fromWCharArray(pe32.szExeFile);
    } while(Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (argc < 3) {
        qDebug() << "Usage: ProcessName ExecuteFile";
        return 0;
    }

    while(true) {
        QStringList processList;
        GetProcessList(processList);

        if (!processList.contains(argv[1])) {
            QProcess *process = new QProcess();
            QString program = argv[2];
            QString folder = QFileInfo(argv[2]).path();
            process->start(program, QStringList() << folder);
        }
        QThread::msleep(500);
    }

    return a.exec();
}
