#include "WzJavaService.h"

WzJavaService::WzJavaService(QObject *parent): QObject(parent)
{

}

void WzJavaService::connectWiFi(const QString &wifiName, const QString &wifiPassword)
{
    qDebug() << "JavaService::startService";
#ifdef Q_OS_ANDROID
    QAndroidIntent serviceIntent(QtAndroid::androidActivity().object(),
                                        "bio/shenhua/WiFiService");
    serviceIntent.putExtra("wifiName", wifiName.toUtf8());
    serviceIntent.putExtra("wifiPassword", wifiPassword.toUtf8());
    QAndroidJniObject result = QtAndroid::androidActivity().callObjectMethod(
                "startService",
                "(Landroid/content/Intent;)Landroid/content/ComponentName;",
                serviceIntent.handle().object());

#endif
}
