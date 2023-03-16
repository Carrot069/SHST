#ifndef SHSTLOGTOQML_H
#define SHSTLOGTOQML_H

#include <QObject>
#include <QList>
#include <QDateTime>

class ShstLogToQml : public QObject
{
    Q_OBJECT
public:
    explicit ShstLogToQml(QObject *parent = nullptr);
    ~ShstLogToQml() override;
    void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    Q_INVOKABLE static QString nowStr();

    static void install();
    static QList<ShstLogToQml*> instances;
    static QtMessageHandler previousHandler;
signals:
    void message(QtMsgType type, const QString &msg);
};

#endif // SHSTLOGTOQML_H
