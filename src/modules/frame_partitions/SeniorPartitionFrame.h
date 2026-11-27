#ifndef SENIORPARTITIONFRAME_H
#define SENIORPARTITIONFRAME_H

#include "PartitionFrameBase.h"
#include "structs.h"
#include "SeniorLVMFrame.h"
#include <DSuggestButton>
#include <DLabel>
#include <DCheckBox>
#include <QStackedWidget>

class FramePluginInterface;
class SeniorPartationListView;
class ItemModel;
class TitleWidget;
class QPushButton;
class QLabel;
class CreateNewPartitionDialog;
class CreateNewLVMDialog;
class EditPartitionDialog;
class HelpLabel;
class SeniorTipDialog;
DWIDGET_USE_NAMESPACE

class SeniorPartitionFrame : public PartitionFrameBase {
    Q_OBJECT
public:
    explicit SeniorPartitionFrame(QWidget *parent = Q_NULLPTR);

    void updateTs() override;

    bool switchBack();

    void initDeviceInfo(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs, const QMap<SVGInfo, QList<SLVMInfo>> &vgs);
signals:
    void  resetList();
private:
    void initConnection();
    void updateInstallInfo(const QMap<SDeviceInfo, QList<SPartitionInfo> > &devs, const QMap<SVGInfo, QList<SLVMInfo> > &vgs);
    void updateTips(const QString &text);
    void setFormatDisk(const SDeviceInfo &dev);

    bool filterGhostDisk(const QString &devPath);
private slots:
    void encryptSinglePartition(const QString &partId);

    void decryptSinglePartition(const QString &partId, const QString &mountPoint);
private:
    QStackedWidget *m_centerWidget = Q_NULLPTR;
    TitleWidget *m_titleWidget = Q_NULLPTR;
    DLabel *m_tipLabel = Q_NULLPTR;
    DSuggestButton *m_nextBtn = Q_NULLPTR;
    QPushButton *m_listReset = Q_NULLPTR;
    QPushButton *m_lvmCreate = Q_NULLPTR;
    HelpLabel *m_helpLabel = Q_NULLPTR;
    SeniorTipDialog *m_helpShowDialog = Q_NULLPTR;
    SeniorPartationListView *m_tableView = Q_NULLPTR;
    ItemModel *m_model = Q_NULLPTR;
    SeniorLVMFrame *m_lvmFrame = Q_NULLPTR;
    DCheckBox *m_encryptRoot = Q_NULLPTR;
    DLabel *m_encryptionTip = Q_NULLPTR;

};

#endif // SENIORPARTITIONFRAME_H

