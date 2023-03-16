#ifndef WZI18N_H
#define WZI18N_H

#include <QObject>
#include <QTranslator>
#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QDebug>
#include <QQmlEngine>
#include <QQmlContext>

class WzI18N: public QObject {
    Q_OBJECT
private:
    bool m_systemLanguageIsZh;
    QString m_fontName;
    QString getLanguage();
    bool getSystemLanguageIsZh() const;
    bool getIsZh() const;

    QFont font() const;
public:
    explicit WzI18N(QObject* parent = nullptr);
    Q_PROPERTY(QFont font READ font NOTIFY fontChanged)
    Q_PROPERTY(QString language READ getLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool isZh READ getIsZh NOTIFY isZhChanged)
    Q_PROPERTY(bool systemLanguageIsZh READ getSystemLanguageIsZh)
    Q_INVOKABLE void switchLanguage(const QString& language, bool isRetranslate = true);

signals:
    void languageChanged(const QString& language);
    void isZhChanged(const bool& isZh);
    void fontChanged();
};

extern int sourceHanSansSCMediumFontId;

#endif // WZI18N_H
