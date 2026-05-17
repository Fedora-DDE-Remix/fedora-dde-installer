#include "MutexObject.h"

QMutex MutexObject::m_mutex;
MutexObject::MutexObject()
{
    m_mutex.lock();
}

MutexObject::~MutexObject()
{
    m_mutex.unlock();
}
