#include "ThreadObject.h"

ThreadObject::ThreadObject(QObject *parent):
    QThread(parent)
{

}

void ThreadObject::setHandle(const std::function<void ()> &handle)
{
    m_handle = handle;
}

void ThreadObject::run()
{
    m_handle();
}
