#include "WzI18N.h"

QString g_language; // 2020-07-28 23:51:18 不能先预置中文, 否则设置中文字体的代码就运行不到了
QFont g_font;
int sourceHanSansSCMediumFontId;
QTranslator m_translator;

QFont WzI18N::font() const
{
    return g_font;
}

WzI18N::WzI18N(QObject *parent)
{
    QString s = QLocale::system().name();
    m_systemLanguageIsZh = s.startsWith("zh", Qt::CaseInsensitive);
}

QString WzI18N::getLanguage()
{
    return g_language;
}

bool WzI18N::getSystemLanguageIsZh() const
{
    return m_systemLanguageIsZh;
}

bool WzI18N::getIsZh() const
{
    return g_language == "zh";
}

void WzI18N::switchLanguage(const QString &language, bool isRetranslate)
{
    if (language == g_language)
        return;
    g_language = language;
    emit languageChanged(language);
    emit isZhChanged(getIsZh());
    qApp->removeTranslator(&m_translator);
    m_translator.load(QLocale(language), "i18n", "_", ":/translations");
    qApp->installTranslator(&m_translator);

    QFont font;
    if (language == "zh") {
        QFontDatabase fontDatabase;
        auto allFontFamilies = fontDatabase.families();
        // 此处获取到的字体名称不一定是出现在字体名称列表中的,
        // 比如在中文Windows系统中, 获取到的仍然是 字体的英文名称 Source Han Sans SC Medium,
        // 但是出现在字体名称列表中和起作用的却是 "思源黑体 Medium"
        auto fontFamilies = QFontDatabase::applicationFontFamilies(sourceHanSansSCMediumFontId);
        fontFamilies.append("思源黑体 Medium");
        if (fontFamilies.count() > 0) {
            foreach(QString s, fontFamilies) {
                if (allFontFamilies.contains(s)) {
                    font.setFamily(s);
                    break;
                }
            }
        }
    } else if (language == "en") {
        font.setFamily("Arial");
    }
    qApp->setFont(font);
    g_font = font;
    emit fontChanged();

    if (isRetranslate) {
        QQmlEngine *ownerEngine = QQmlEngine::contextForObject(this)->engine();
        ownerEngine->retranslate();
    }
}
