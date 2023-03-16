#ifndef WZI18N_H
#define WZI18N_H

#include <QObject>
#include <QTranslator>
#include <QApplication>
#include <QFont>
#include <QDebug>
#include <QQmlEngine>
#include <QQmlContext>
#include <QFontDatabase>

class WzI18N: public QObject {
    Q_OBJECT
private:
    QString getLanguage();
    QFont font() const;
public:
    Q_PROPERTY(QString language READ getLanguage NOTIFY languageChanged)
    Q_INVOKABLE void switchLanguage(const QString& language, bool isRetranslate = true);
    Q_PROPERTY(QFont font READ font NOTIFY fontChanged)
signals:
    void languageChanged(const QString& language);
    void fontChanged();
};

extern int fontIdSourceHanSansSCMedium;

#endif // WZI18N_H
