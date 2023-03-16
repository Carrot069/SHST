#include "WzI18N.h"

QString g_language; // 2020-07-28 23:51:18 不能先预置中文, 否则设置中文字体的代码就运行不到了
QTranslator m_translator;

QString WzI18N::getLanguage()
{
    return g_language;
}

void WzI18N::switchLanguage(const QString &language, bool isRetranslate)
{
    if (language == g_language)
        return;
    g_language = language;
    emit languageChanged(language);
    qApp->removeTranslator(&m_translator);
    m_translator.load(QLocale(language), "i18n", "_", ":/translations");
    qApp->installTranslator(&m_translator);

    QFont font;
    if (language == "zh") {
        font.setFamily("思源黑体 Medium");
    } else if (language == "en") {
        font.setFamily("Arial");
    }
    qApp->setFont(font);

    if (isRetranslate) {
        QQmlEngine *ownerEngine = QQmlEngine::contextForObject(this)->engine();
        ownerEngine->retranslate();
    }
}
