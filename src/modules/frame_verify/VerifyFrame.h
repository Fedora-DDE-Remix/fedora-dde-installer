#ifndef DEEPIN_INSTALLER_VERIFYFRAME_H
#define DEEPIN_INSTALLER_VERIFYFRAME_H

#include "FramePluginInterface.h"
#include "IsoVerify.h"
#include "globals.h"
#include <DSpinner>
#include <DSuggestButton>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

class TitleWidget;
class QLabel;

class VerifyFrame : public FramePluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    explicit VerifyFrame(QWidget *parent = nullptr);
    bool load();
    void startVerify();

private slots:
    void onStart();
    void onVerifyFinished(bool success);
protected:
    void showEvent(QShowEvent *event) override;
private:
    void initUI();
    void updateTs() override;

private:
    ISOVerify* m_isoVerify = nullptr;
    TitleWidget* m_titleWidget = nullptr;
    DSpinner* m_spinner = nullptr;
    QLabel* m_resultImage = nullptr;
    QLabel* m_tipsVerifying = nullptr;
    QLabel* m_tipsVerifySuccess = nullptr;
    QLabel* m_tipsVerifyFailed = nullptr;
    DSuggestButton *m_nextBnt = nullptr;
};


#endif //DEEPIN_INSTALLER_VERIFYFRAME_H
