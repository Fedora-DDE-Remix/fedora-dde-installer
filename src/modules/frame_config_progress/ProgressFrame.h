#ifndef PROGRESSFRAME_H
#define PROGRESSFRAME_H

#include "FramePluginInterface.h"

class AnimationWidget;
class WarningDialog;
class QLabel;

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

private slots:
    void warnWidgetEnterBtnSlot();

private:
    WarningDialog *m_warningFrame = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_progLabel = nullptr;
};

#endif // PROGRESSFRAME_H
