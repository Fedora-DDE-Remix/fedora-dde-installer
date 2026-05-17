#ifndef SENIORLVMFRAME_H
#define SENIORLVMFRAME_H

#include "FramePluginInterface.h"
#include "structs.h"

#include <DPushButton>
#include <DSuggestButton>
#include <DLabel>
#include <DLineEdit>

class FramePluginInterface;
class SeniorLVMListView;
class LVMItemModel;
class TitleWidget;
class QLabel;
class CreateNewPartitionDialog;
class CreateNewLVMDialog;
class SeniorLVMFrame : public QWidget {
    Q_OBJECT

public:
    explicit SeniorLVMFrame(QWidget *parent = Q_NULLPTR);

    void setDevData(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs);

    void updateTs();
signals:
    void createLVM();

    void back();
private:
    TitleWidget *m_titleWidget = Q_NULLPTR;
    DPushButton *m_cancelBtn = Q_NULLPTR;
    DSuggestButton *m_okBtn = Q_NULLPTR;
    DLabel *m_lvmCreate = Q_NULLPTR;
    DLabel *m_group = Q_NULLPTR;
    DLineEdit *m_groupCreate = Q_NULLPTR;
    DLabel *m_tipLabel = Q_NULLPTR;

    SeniorLVMListView *m_tableView = Q_NULLPTR;
    LVMItemModel *m_model = Q_NULLPTR;
    QMap<SDeviceInfo, QList<SPartitionInfo>> m_devs;

    CreateNewPartitionDialog *m_newPartitionDlg = Q_NULLPTR;
    CreateNewLVMDialog *m_newLVMDlg = Q_NULLPTR;

private:
    void updateTips(const QString &text);
};

#endif // SENIORLVMFRAME_H

