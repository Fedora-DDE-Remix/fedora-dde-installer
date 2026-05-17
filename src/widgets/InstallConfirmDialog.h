#ifndef INSTALLCONFIGDIALOG_H
#define INSTALLCONFIGDIALOG_H

#include <DDialog>
#include <DPushButton>
#include <DCheckBox>
#include <DBlurEffectWidget>
#include <DLabel>

class HelpLabel;
class BackupTipDialog;
DWIDGET_USE_NAMESPACE
class InstallConfirmDialog : public DDialog {
    Q_OBJECT
public:
    explicit InstallConfirmDialog(QWidget *parent = nullptr);

    ~InstallConfirmDialog();
    void setOperatorInfo(const QString &info);
    void setInitRecovery(bool isRecovery);

private:
    DLabel *m_operatorInfo = nullptr;
    DPushButton *m_startInstallBtn = nullptr;
    DCheckBox *m_isInitRecovery = nullptr;
    HelpLabel *m_helpLabel = nullptr;
    BackupTipDialog *m_helpDialog = nullptr;
};

#endif // INSTALLCONFIGDIALOG_H
