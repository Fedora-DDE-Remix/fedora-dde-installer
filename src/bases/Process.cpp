#include "Process.h"

#include <QDebug>

Process::Process(QObject *parent)
{
    connect(this, &Process::readyReadStandardOutput, this, &Process::onReadInfo);
    connect(this, &Process::readyReadStandardError, this, &Process::onReadWarning);
}

QByteArray Process::readAllStandardOutput()
{
    return mInfo;
}

QByteArray Process::readAllStandardError()
{
    return mError;
}

void Process::onReadInfo()
{
    QByteArray curentLog = QProcess::readAllStandardOutput();
    qInfo() << QString(qPrintable(curentLog)).trimmed().toUtf8().constData();
    mInfo += curentLog;
}

void Process::onReadWarning()
{
    QByteArray curentLog = QProcess::readAllStandardError();
    qWarning() << QString(qPrintable(curentLog)).trimmed().toUtf8().constData();
    mError += curentLog;
}
