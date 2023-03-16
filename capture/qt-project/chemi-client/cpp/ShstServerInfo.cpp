#include "ShstServerInfo.h"

ShstServerInfo::ShstServerInfo(QObject *parent)
    : QObject{parent}
{
    qInfo() << "ShstServerInfo";
}

ShstServerInfo::~ShstServerInfo()
{
    qInfo() << "~ShstServerInfo";
}

QString ShstServerInfo::serverHost() const
{
    return WzUdpBroadcastReceiver::ServerHost;
}

int ShstServerInfo::serverPort() const
{
    return WzUdpBroadcastReceiver::ServerPort;
}

int ShstServerInfo::fileServerPort() const
{
    return WzUdpBroadcastReceiver::FilePort2;
}
