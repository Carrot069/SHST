#include "ShstLogToQml.h"

QtMessageHandler ShstLogToQml::previousHandler = nullptr;
QList<ShstLogToQml*> ShstLogToQml::instances = QList<ShstLogToQml*>();

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    foreach (auto a, ShstLogToQml::instances) {
        a->messageHandler(type, context, msg);
    }
}

ShstLogToQml::ShstLogToQml(QObject *parent)
    : QObject{parent}
{
    instances.append(this);
}

ShstLogToQml::~ShstLogToQml()
{
    instances.removeAll(this);
}

void ShstLogToQml::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    bool isOutput = true;
    if (type == QtDebugMsg) {
        #ifndef QT_DEBUG
        isOutput = false;
        #endif
    }
    if (isOutput) {
        QString showMsg = msg;
        #ifdef QT_DEBUG
        showMsg += QString("(%1,%2,%3)").arg(context.function, context.file).arg(context.line);
        #endif
        emit message(type, showMsg);
    }
    if (previousHandler)
        previousHandler(type, context, msg);
}

QString ShstLogToQml::nowStr()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

void ShstLogToQml::install()
{
    previousHandler = qInstallMessageHandler(messageOutput);
}
