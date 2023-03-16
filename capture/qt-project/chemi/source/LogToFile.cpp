#include "LogToFile.h"

std::shared_ptr<spdlog::logger> loggerMain;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
#ifdef QT_DEBUG
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
#else
    Q_UNUSED(context)
#endif
    switch (type) {
    case QtDebugMsg:
#ifdef QT_DEBUG
        loggerMain->debug("{0} ({1}:{2}, {3})\n", localMsg.constData(), file, context.line, function);
#endif
        break;
    case QtInfoMsg:
#ifdef QT_DEBUG
        loggerMain->info("{0} ({1}:{2}, {3})", localMsg.constData(), file, context.line, function);
#else
        loggerMain->info(localMsg.constData());
#endif
        break;
    case QtWarningMsg:
#ifdef DEBUG
        loggerMain->warn("{0} ({1}:{2}, {3})", localMsg.constData(), file, context.line, function);
#else
        loggerMain->warn(localMsg.constData());
#endif
        break;
    case QtCriticalMsg:
#ifdef QT_DEBUG
        loggerMain->critical("{0} ({1}:{2}, {3})", localMsg.constData(), file, context.line, function);
#else
        loggerMain->critical(localMsg.constData());
#endif
        break;
    case QtFatalMsg:
#ifdef QT_DEBUG
        loggerMain->error("{0} ({1}:{2}, {3})", localMsg.constData(), file, context.line, function);
#else
        loggerMain->error(localMsg.constData());
#endif
        break;
    }
}

void LogToFileInit() {
   auto max_size = 1024 * 1024 * 10; // 10MB
   auto max_files = 10;
   auto log_file = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
           "/" + kOrganizationName + "/" + kApplicationName + "/" + kApplicationName + ".log";
   loggerMain = spdlog::rotating_logger_mt("main", log_file.toStdString(), max_size, max_files);

   qInstallMessageHandler(myMessageOutput);
}

