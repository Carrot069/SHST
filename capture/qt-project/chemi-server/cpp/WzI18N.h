#ifndef WZI18N_H
#define WZI18N_H

#include <QObject>
#include <QTranslator>
#include <QApplication>
#include <QFont>
#include <QDebug>
#include <QQmlEngine>
#include <QQmlContext>

class WzI18N: public QObject {
    Q_OBJECT
private:
    QString getLanguage();
public:
    Q_PROPERTY(QString language READ getLanguage NOTIFY languageChanged)
    Q_INVOKABLE void switchLanguage(const QString& language, bool isRetranslate = true);

signals:
    void languageChanged(const QString& language);
};

#endif // WZI18N_H
