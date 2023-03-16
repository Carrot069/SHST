#ifndef WZRENDER_H
#define WZRENDER_H

#include <QObject>
#include <QCoreApplication>
#include <QProcess>

#ifdef HARDLOCK
#include "RY3_API.h"
#endif

class WzRender : public QObject
{
    Q_OBJECT
public:
    explicit WzRender(QObject *parent = nullptr);

    Q_INVOKABLE int getCount();
    Q_INVOKABLE void restart();

signals:

};

#endif // WZRENDER_H
