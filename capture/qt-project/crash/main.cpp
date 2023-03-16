#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "StartProcess.h"
#include "WzI18N.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);

    StartProcess startProcess;
    if (argc >= 2) {
        startProcess.setExeName(argv[1]);
    }

    QString fontFileName = app.applicationDirPath() + "/SourceHanSansSC-Medium.otf";
    fontIdSourceHanSansSCMedium = QFontDatabase::addApplicationFont(app.applicationDirPath() + "/SourceHanSansSC-Medium.otf");

    {
        // 在加载任何qml之前安装语言, 防止程序运行慢的时候出现了qml界面后再切换语言，那样会看到切换前的语言
        QString languageName = "zh";
        if (argc >= 3) {
            languageName = argv[2];
        }
        WzI18N i18n;
        i18n.switchLanguage(languageName, false);
    }

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);
    engine.rootContext()->setContextProperty("startProcess", &startProcess);

    return app.exec();
}
