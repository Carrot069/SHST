#include "WzSingleton.h"

template <class T>
T* Singleton<T>::Instance()
{
    static QMutex mutex;
    static QScopedPointer<T> inst;
    if (Q_UNLIKELY(!inst)) {
        mutex.lock();
        if (!inst) {
            inst.reset(new T);
        }
        mutex.unlock();
    }
    return inst.data();
}
