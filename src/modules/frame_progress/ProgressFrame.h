#ifndef PROGRESSFRAME_H
#define PROGRESSFRAME_H

#include "FramePluginInterface.h"

class ProgressUpdateFrame;
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

private:
    ProgressUpdateFrame *m_progressUpdateWidget = nullptr;
    bool m_running = false;
    ProgressFailedFrame *m_progressFailedWidget = nullptr;
    QStackedLayout *m_mainLayout = nullptr;
};

#endif // PROGRESSFRAME_H
