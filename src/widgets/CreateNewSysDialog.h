#ifndef CREATENEWSYSDIALOG_H
#define CREATENEWSYSDIALOG_H

#include "structs.h"

#include <DDialog>
#include <DSlider>
#include <DLabel>
#include <DPushButton>
#include <DPasswordEdit>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class TitleWidget;
class PartitionSizeSlider;
class CreateNewSysDialog : public DDialog
{
    Q_OBJECT
public:
    explicit CreateNewSysDialog(const SPartitionInfo &free, QWidget *parent = Q_NULLPTR);

    void setSystemSpaceRange(const qint64 &Minimum, const qint64 &Maximum);
    void setDataSpaceRange(const qint64 &Minimum, const qint64 &Maximum);
    qint64 getSystemSpaceSize();
    qint64 getDataSpaceSize();
    bool getFormatPartitionCheckStatus();

private:
    TitleWidget *m_titleWidget = Q_NULLPTR;
    DLabel *m_systemLabel = Q_NULLPTR;
    DLabel *m_dataLabel = Q_NULLPTR;
    PartitionSizeSlider *m_editSystemSlider = Q_NULLPTR;
    PartitionSizeSlider *m_editDataSlider = Q_NULLPTR;
    DPushButton *m_cancelBtn = Q_NULLPTR;
    DSuggestButton *m_okBtn = Q_NULLPTR;
    DCheckBox *m_formatPartition = Q_NULLPTR;
    DLabel *m_tipsLabel = Q_NULLPTR;

    SPartitionInfo m_freePartitionInfo;

private:
    void initConnection();
    void updateErrTips(const QString &err);

};

#endif // CREATENEWSYSDIALOG_H
