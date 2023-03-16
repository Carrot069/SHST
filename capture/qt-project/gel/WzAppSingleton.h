#ifndef WZAPPSINGLETON_H
#define WZAPPSINGLETON_H

#include <QObject>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "WzGlobalConst.h"

class WzAppSingleton : public QObject
{
    Q_OBJECT
private:
#ifdef Q_OS_WIN
    HANDLE m_mutexHandle;
#endif
    bool m_appInstanceAlready = false;
public:
    explicit WzAppSingleton(QObject *parent = nullptr);
    ~WzAppSingleton() override;
    bool appInstanceAlready() const;

    Q_PROPERTY(bool appInstanceAlready READ appInstanceAlready CONSTANT)

signals:

};

#endif // WZAPPSINGLETON_H
