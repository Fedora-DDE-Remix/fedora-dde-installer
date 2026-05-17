#ifndef EDITPARTATIONDIALOG_H
#define EDITPARTATIONDIALOG_H

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
class EditPartitionDialog : public DDialog
{
    Q_OBJECT
public:
    explicit EditPartitionDialog(const SPartitionInfo &info, QWidget *parent = Q_NULLPTR);
    SPartitionInfo getPartition() const;
    bool getFormatPartitionCheckStatus();

private:
    void updateErrTips(const QString &err);
    QStringList getFileSystemList();
    QStringList getMountPointList();
    bool canUseEFI(const SPartitionInfo &info);

private:
    TitleWidget *m_titleWidget = Q_NULLPTR;
    DLabel *m_fileSystemLabel = Q_NULLPTR;
    DComboBox *m_fileSystemCmb = Q_NULLPTR;
    DLabel *m_mountPointLabel = Q_NULLPTR;
    DComboBox *m_mountPointCmb = Q_NULLPTR;
    DCheckBox *m_formatPartition = Q_NULLPTR;
    DLabel *m_tipsLabel = Q_NULLPTR;
    QAbstractButton *m_cancelBtn = Q_NULLPTR;
    QAbstractButton *m_okBtn = Q_NULLPTR;

    SPartitionInfo m_editPartitionInfo;
    QString m_formattedTips;
};

#endif // EDITPARTATIONDIALOG_H
