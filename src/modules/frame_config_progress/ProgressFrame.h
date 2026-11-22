#ifndef FEDORA_DDE_CONFIGURATION_PROGRESS_FRAME_H
#define FEDORA_DDE_CONFIGURATION_PROGRESS_FRAME_H

#include "FramePluginInterface.h"

class QLabel;
class QPushButton;
class QProgressBar;

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
    void exitInstall(const QString &error);
    void actionClicked();

    QLabel *m_logoLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_progLabel = nullptr;
    QProgressBar *m_progressWidget = nullptr;
    QPushButton *m_actionButton = nullptr;
    QPushButton *m_secondaryButton = nullptr;
    bool m_running = false;
    bool m_complete = false;
    bool m_accountSaved = false;
    QString m_error;
};

#endif
