#ifndef SINGLETON_H
#define SINGLETON_H

#include <QMutex>
#include <QScopedPointer>

template <class T>
class Singleton
{
public:
    static T* Instance();
};

#endif // SINGLETON_H
