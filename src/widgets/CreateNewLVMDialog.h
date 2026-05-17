#ifndef CREATENEWLVMDIALOG_H
#define CREATENEWLVMDIALOG_H

#include "structs.h"

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DPasswordEdit>
#include <DSuggestButton>
#include <DComboBox>

DWIDGET_USE_NAMESPACE

class PartitionSizeSlider;
class TitleWidget;
class CreateNewLVMDialog : public DDialog
{
    Q_OBJECT
public:
    explicit CreateNewLVMDialog(const SLVMInfo &freeLv, QWidget *parent = Q_NULLPTR);
    SLVMInfo getLVInfo() const;
    bool getFormatPartitionCheckStatus();

private:
    void initParition();
    bool limitEditorSizeByFileSystem();
    bool limitEditorSizeByMountPoint();
    void updateErrTips(const QString &err);
    QStringList getFileSystemList();

    bool checkSliderRange(qint64 minSize, qint64 maxSize, bool bUseMin);

private:
    TitleWidget *m_titleWidget = Q_NULLPTR;
    DLabel *m_fileSystemLabel = Q_NULLPTR;
    DComboBox *m_fileSystemCmb = Q_NULLPTR;
    DLabel *m_mountPointLabel = Q_NULLPTR;
    DComboBox *m_mountPointCmb = Q_NULLPTR;
    DLabel *m_sizeLabel = Q_NULLPTR;
    DLabel *m_tipsLabel = Q_NULLPTR;
    PartitionSizeSlider *m_editSlider = Q_NULLPTR;
    QAbstractButton *m_cancelBtn = Q_NULLPTR;
    QAbstractButton *m_okBtn = Q_NULLPTR;
    DCheckBox *m_formatPartition = Q_NULLPTR;

    SLVMInfo m_newLVMInfo;
    SLVMInfo m_freeLVMInfo;
};

#endif // CREATENEWLVMDIALOG_H
