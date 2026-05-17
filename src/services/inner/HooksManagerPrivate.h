#ifndef HOOKSMANAGERPRIVATE_H
#define HOOKSMANAGERPRIVATE_H

#include "structs.h"

#include <QObject>
#include <QList>

class HooksManager;

class HooksManagerPrivate : public QObject
{
    Q_OBJECT
    friend HooksManager;

signals:
    void finished();
    void exitInstall(const QString &err);
    void update();

private:
    void install();
    int  jobSize();      // 脚本的数量

private:
    QList<SHooksInfo> m_workspacesHooks; // 记录每个阶段的脚本列表
    int hooksPos = 0;   // 记录当前执行的脚本的位置
    int m_hooksStep = 1;      // 记录当前执行的阶段
};

#endif // HOOKSMANAGERPRIVATE_H
