#ifndef MULTIPLEDISKPARTITIONFRAME_H
#define MULTIPLEDISKPARTITIONFRAME_H

#include "PartitionFrameBase.h"
#include "structs.h"
#include "HelpRadioButton.h"

#include <DSuggestButton>
#include <DToolButton>
#include <DLabel>
#include <DRadioButton>
#include <DDialog>
#include <DListView>
#include <DComboBox>
#include <QWidget>
#include <DTipLabel>

class TitleWidget;
class SizeEditDialog;
class HelpDialog;
class HelpLabel;
class MultipleDiskItem;
class DeviceListView;
class MultemDelegate;
DWIDGET_USE_NAMESPACE

class MultipleDiskPartitionFrame : public PartitionFrameBase {
    Q_OBJECT
public:
    explicit MultipleDiskPartitionFrame(QWidget *parent = Q_NULLPTR);
    void resetItemCheckedStatus(const QModelIndex &previous, bool bflag);

    void updateTs() override;

    void initDeviceInfo(const QList<SDeviceInfo> &infos);
protected:
    void showEvent(QShowEvent *event) override;
protected slots:
    void itemClicked(const QModelIndex &currentIndex);
    void showAutoSelectDialog();

    void onEditSize();

    void onNextButtonClicked();

    void handleSystemDiskChange(const QModelIndex &current, const QModelIndex &previous);
signals:
    void  enter();
    void  leave();

    void decryptSuccess();
private:
    void initContent();

    void initConnection();

    bool checkDiskIsOK();

    bool checkDiskNeedDecrypt(bool &decryptStatus);

    bool handleDecrpytDisk(const QList<SPartitionInfo> &devList);

    void updateSystemDiskStatus();

    void adjustSystemSize(qint64 sysSize, bool bEnable);

    void updateListState();
    void setControlEnable();
    bool updateDiskInfo(const SDeviceInfo &info);
    void updateDiskInfo(const SDeviceInfo &info, EDevType type);
    void updateTipInfo(const QString &text);
    bool checkSysDiskSpace(const SDeviceInfo &info);
    SDeviceInfo getItemDevInfo(int row);
    MultipleDiskItem *getItemInfo(int row);

private:
    QVBoxLayout *m_mainlayout = nullptr;
    DSuggestButton *m_nextBtn = Q_NULLPTR;
    TitleWidget *m_titleWidget = Q_NULLPTR;

    QListView * m_sysDevView = Q_NULLPTR;
    QStandardItemModel *m_devInfoListModel = Q_NULLPTR;
    DComboBox *m_dataDevCombo = Q_NULLPTR;
    QButtonGroup *m_modeGroup = Q_NULLPTR;

    qint64 m_sysMinSize; //以Mib为单位

    DToolButton *m_editbtn = Q_NULLPTR;
    DLabel *m_tipLabel = Q_NULLPTR;
    DLabel *m_labelSys = Q_NULLPTR;
    DLabel *m_dataLabel = Q_NULLPTR;
    DLabel *m_labelMode = Q_NULLPTR;
    HelpRadioButton *m_btnDefault = Q_NULLPTR;
    DTipLabel *m_modeTip_1 = Q_NULLPTR;
    HelpRadioButton *m_btnEncrypt;
    DTipLabel *m_modeTip_2 = Q_NULLPTR;
    HelpRadioButton *m_btnExtand = Q_NULLPTR;
    DTipLabel *m_modeTip_3 = Q_NULLPTR;
    HelpRadioButton *m_btnKeepData = Q_NULLPTR;
    DTipLabel *m_modeTip_4 = Q_NULLPTR;
    DTipLabel *m_modeTip_5 = Q_NULLPTR;

    SizeEditDialog *m_sizeEditDialog = Q_NULLPTR;
    QList<SDeviceInfo> m_devs = {};
    QString m_fullEncryptionPwd = Q_NULLPTR;
    bool m_autoDecrypt = false;

    QMap<QString, EDevType> m_deviceType;

    QWidget *m_contentWidget = Q_NULLPTR;
};

#endif // MULTIPLEDISKPARTITIONFRAME_H
