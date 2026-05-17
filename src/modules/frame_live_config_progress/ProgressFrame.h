#ifndef PROGRESSFRAME_H
#define PROGRESSFRAME_H

#include "FramePluginInterface.h"
#include "TitleWidget.h"

class AnimationWidget;
class WarningDialog;

class ProgressFream : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    explicit ProgressFream(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void updateTs() override;
    // 覆盖父类方法，进度界面不需要上一步和关闭按钮
    void initTitleButton(){};

private:
    void startInstall();
    void finished();
    void exitInstall(const QString &err);

private slots:
    void warnWidgetEnterBtnSlot();

private:
    WarningDialog *m_warningFrame = nullptr;
    TitleWidget *m_title;
    QLabel *m_progLabel;
};

#endif // PROGRESSFRAME_H
