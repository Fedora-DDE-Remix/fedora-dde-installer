#ifndef CUSTOMPARTITION_H
#define CUSTOMPARTITION_H

#include "PartitionFrameBase.h"
#include "structs.h"

#include <DSuggestButton>
#include <DLabel>

class FramePluginInterface;
class TitleWidget;
class CustomPartationListView;
class DataModel;
class CreateNewSysDialog;
class EditPartitionDialog;
DWIDGET_USE_NAMESPACE

class CustomPartitionFrame : public PartitionFrameBase {
    Q_OBJECT
public:
    explicit CustomPartitionFrame(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs, const QMap<SVGInfo, QList<SLVMInfo>> &vgs, QWidget *parent = nullptr);

    void updateTs() override;

    void initDeviceInfo(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs, const QMap<SVGInfo, QList<SLVMInfo>> &vgs);
private:
    void updateInstallInfo(const QMap<SDeviceInfo, QList<SPartitionInfo> > &devs, const QMap<SVGInfo, QList<SLVMInfo> > &vgs);
    void updateTips(const QString &text);
    void setFormatDisk(const SDeviceInfo &dev);
private:
    TitleWidget *m_titleWidget = Q_NULLPTR;
    DSuggestButton *m_nextBtn = Q_NULLPTR;

    CustomPartationListView *m_tableView = Q_NULLPTR;
    DataModel *m_model = Q_NULLPTR;
    QMap<SDeviceInfo, QList<SPartitionInfo>> m_devs;
    QMap<SVGInfo, QList<SLVMInfo>> m_vgs;
    DLabel *m_tipLabel = Q_NULLPTR;
};

#endif // CUSTOMPARTITION_H
