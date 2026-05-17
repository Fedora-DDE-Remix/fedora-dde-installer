#ifndef THREADOBJECT_H
#define THREADOBJECT_H

#include <QThread>
#include <functional>

class ThreadObject : public QThread {

public:
    ThreadObject(QObject *parent = nullptr);
    void setHandle(const std::function<void()> &handle);

protected:
    void run() override;

private:
    std::function<void()> m_handle;
};

#endif // THREADOBJECT_H
