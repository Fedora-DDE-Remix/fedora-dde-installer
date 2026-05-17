#ifndef PROCESS_H
#define PROCESS_H

#include <QProcess>

class Process : public QProcess
{
    Q_OBJECT
public:
    Process(QObject *parent = nullptr);

public:
    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

private slots:
    void onReadInfo();
    void onReadWarning();

private:
    QByteArray mInfo;
    QByteArray mError;
};

#endif // PROCESS_H
