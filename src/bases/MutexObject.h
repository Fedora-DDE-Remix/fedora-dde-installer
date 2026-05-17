#ifndef MUTEXOBJECT_H
#define MUTEXOBJECT_H

#include <QMutex>

class MutexObject {
public:
    MutexObject();
    ~MutexObject();

private:
    MutexObject(const MutexObject&) = delete;
    MutexObject &operator=(const MutexObject &) = delete;

private:
    static QMutex m_mutex;
};

#endif // MUTEXOBJECT_H
