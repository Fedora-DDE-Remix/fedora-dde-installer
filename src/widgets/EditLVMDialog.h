#ifndef EDITLVMDIALOG_H
#define EDITLVMDIALOG_H

#include "structs.h"

#include <DDialog>
#include <DSlider>
#include <DLabel>
#include <DPushButton>
#include <DPasswordEdit>
#include <DSuggestButton>
#include <DComboBox>

DWIDGET_USE_NAMESPACE

class TitleWidget;
class EditLVMDialog : public DDialog
{
    Q_OBJECT
public:
    explicit EditLVMDialog(const SLVMInfo &info, QWidget *parent = Q_NULLPTR);
    SLVMInfo getPartition() const;
    bool getFormatPartitionCheckStatus();

private:
    void updateErrTips(const QString &err);
    QStringList getFileSystemList();

private:
    TitleWidget *m_titleWidget = Q_NULLPTR;
    DLabel *m_fileSystemLabel = Q_NULLPTR;
    DComboBox *m_fileSystemCmb = Q_NULLPTR;
    DLabel *m_mountPointLabel = Q_NULLPTR;
    DCheckBox *m_formatPartition = Q_NULLPTR;
    DLabel *m_tipsLabel = Q_NULLPTR;
    DComboBox *m_mountPointCmb = Q_NULLPTR;
    DPushButton *m_cancelBtn = Q_NULLPTR;
    DSuggestButton *m_okBtn = Q_NULLPTR;

    SLVMInfo m_editPartitionInfo;
};

#endif // EDITLVMDIALOG_H
