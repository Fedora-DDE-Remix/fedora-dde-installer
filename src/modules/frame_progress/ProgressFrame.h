#ifndef PROGRESSFRAME_H
#define PROGRESSFRAME_H

#include "FramePluginInterface.h"

class ProgressUpdateFrame;
class ProgressSuccessFrame;
class ProgressFailedFrame;
class QStackedLayout;

class ProgressFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    explicit ProgressFrame(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void updateTs() override;

private:
    void startInstall();
    void finished();
    void exitInstall(const QString &err);
    void doRecord();
    void updateInstallType(int &installType, bool &cryptSetup, bool &saveData, bool &lvmFullDisk);
    void updateInstallTime(QString &installTime);
    void updateSize(qint64 &rootSize, qint64 &swapSize);
    void updateDiskType(QString &diskType);

private:
    ProgressUpdateFrame *m_progressUpdateWidget = nullptr;
    ProgressSuccessFrame *m_progressSuccessWidget = nullptr;
    ProgressFailedFrame *m_progressFailedWidget = nullptr;
    QStackedLayout *m_mainLayout = nullptr;
};

#endif // PROGRESSFRAME_H
