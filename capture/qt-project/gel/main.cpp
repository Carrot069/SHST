#include <QGuiApplication>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QFont>
#include <QTime>
#include <QMetaType>
#include <QDebug>

#include <QStandardPaths>

#include "WzGlobalEnum.h"
#include "WzLiveImageView.h"
#include "WzImageView.h"
#include "WzCamera.h"
#include "WzCaptureService.h"
#include "WzUtils.h"
#include "WzImageService.h"
#include "WzDatabaseService.h"
#include "WzMcuQml.h"
#include "WzRender.h"
#include "WzRenderThread.h"
#include "WzPseudoColor.h"
#include "Test.h"
#include "WzRenderThread.h"
#include "WzI18N.h"
#include "MiniDump.h"
#include "WzLogger.h"
#include "WzIniSetting.h"
#include "WzAppSingleton.h"

int main(int argc, char *argv[])
{
    //TestCreateThumb();
    //TestImageFilter();
    //TestFitRect();
    qDebug() << "currentSecsSinceEpoch:" << QDateTime::currentSecsSinceEpoch();

    WzLogger logger;
    WzIniSetting iniSetting;
    if (iniSetting.readBool("Normal", "LogFile", false))
        logger.startDbgView();

    qmlRegisterType<LiveImageView>("WzCapture", 1, 0, "LiveImageView");
    qmlRegisterType<WzCaptureService>("WzCapture", 1, 0, "WzCaptureService");
    qmlRegisterType<WzCameraState>("WzCapture", 1, 0, "WzCameraState");
    qmlRegisterType<WzImageService>("WzImage", 1, 0, "WzImageService");
    qmlRegisterType<WzImageView>("WzImage", 1, 0, "WzImageView");
    qmlRegisterType<WzDatabaseService>("WzDatabaseService", 1, 0, "WzDatabaseService");
    qmlRegisterType<WzIniSetting>("WzIniSetting", 1, 0, "WzIniSetting");
    qmlRegisterType<WzMcuQml>("WzCapture", 1, 0, "WzMcu");
    qmlRegisterType<WzEnum>("WzEnum", 1, 0, "WzEnum");
    qmlRegisterType<WzPseudoColor>("WzImage", 1, 0, "WzPseudoColor");

    qRegisterMetaType<WzCameraState::CameraState>("WzCameraState");
    qRegisterMetaType<WzEnum::ImageSource>("WzImageSource");
    qRegisterMetaType<WzEnum::ErrorCode>("WzErrorCode");

    qmlRegisterSingletonType<WzUtils>("WzUtils", 1, 0, "WzUtils", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        WzUtils* wzUtils = new WzUtils();
        return wzUtils;
    });

    qmlRegisterSingletonType<WzI18N>("WzI18N", 1, 0, "WzI18N", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            WzI18N* wzI18N = new WzI18N();
            return wzI18N;
        });

#ifdef HARDLOCK
    qDebug() << "HARDLOCK";
#endif
    qmlRegisterSingletonType<WzRender>("WzRender", 1, 0, "WzRender", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        WzRender* render = new WzRender();
        return render;
    });
    qmlRegisterType<WzRenderThread>("WzRender", 1, 0, "WzRenderThread");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);
    app.setOrganizationName(kOrganizationName);
    app.setOrganizationDomain(kOrganizationDomain);
    app.setApplicationName(kApplicationName);
#ifdef QT_DEBUG
    app.setApplicationDisplayName(QString(kApplicationName) + " " + APP_VERSION + " D");
#else
    app.setApplicationDisplayName(QString(kApplicationName) + " " + APP_VERSION);
#endif

    qInfo() << "Current path: " << QCoreApplication::applicationDirPath();
    QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).mkpath(".");
    QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).mkpath(kPaletteFolder);

#ifdef DEMO
    QDate now = QDate::currentDate();
    if (now.year() != 2022) return 0;
    WzRenderThread rt;
    rt.start();
#endif

#ifdef QT_DEBUG
#ifdef Q_OS_WIN
    InstallMiniDump();
#endif
#endif

    {
        QFontDatabase::addApplicationFont(app.applicationDirPath() + "/digital-dismay.regular.otf");
        sourceHanSansSCMediumFontId = QFontDatabase::addApplicationFont(app.applicationDirPath() + "/SourceHanSansSC-Medium.otf");
        // 2020-07-28 23:30:29
        // 在加载任何qml之前安装语言, 防止程序运行慢的时候出现了qml界面后再切换语言，那样会看到切换前的语言
        WzDatabaseService db;
        QString languageName = db.readStrOption("languageName", "zh");
        WzI18N i18n;
        i18n.switchLanguage(languageName, false);
    }

    WzAppSingleton singleton;

    QQmlApplicationEngine engine;
    const QUrl url(singleton.appInstanceAlready() ?
                        QStringLiteral("qrc:/CanNotUse.qml") :
                        QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
