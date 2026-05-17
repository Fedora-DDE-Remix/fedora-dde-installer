#ifndef FULLDISKPARTITIONFRAME_H
#define FULLDISKPARTITIONFRAME_H

#include "PartitionFrameBase.h"
#include "structs.h"
#include "SizeSliderWidget.h"
#include "HelpLabel.h"
#include "HelpRadioButton.h"
#include <DSuggestButton>
#include <DLabel>
#include <DPushButton>
#include <DRadioButton>
#include <DDialog>
#include <DSpinBox>
#include <DTipLabel>

DWIDGET_USE_NAMESPACE

class FramePluginInterface;
class DiskDeviceButton;
class DiskinfoWidget;
class TitleWidget;
class QButtonGroup;
class SizeEditDialog;
class HelpDialog;
class HelpLabel;

class FullDiskPartitionFrame : public PartitionFrameBase {
    Q_OBJECT
public:
    explicit FullDiskPartitionFrame(QWidget *parent = nullptr);

    void initDiskData(const QList<SDeviceInfo> &devs);

    void updateTs() override;
protected:
    void showEvent(QShowEvent *event) override;
signals:
    void  enter();
    void  leave();

    void decryptSuccess();
private slots:
    void onNextBtnClicked();

private:
    void initContent();

    void initConnection();

    void resetToDefaultSize();

    bool checkDiskSpace();

    bool checkDiskNeedDecrypt(bool &decryptStatus);

    bool handleDecryptDisk(const QList<SPartitionInfo> &devList);
private:
    QVBoxLayout *m_mainlayout = nullptr;
    DSuggestButton *m_nextBtn = Q_NULLPTR;
    TitleWidget *m_titleWidget = Q_NULLPTR;

    DLabel *m_tipLabel = Q_NULLPTR;
    DLabel *m_labelDiskName = Q_NULLPTR;
    DLabel *m_labelDiskSize = Q_NULLPTR;
    DLabel *m_labelDiskPath = Q_NULLPTR;
    SizeSliderWidget *m_diskSizeSlider = Q_NULLPTR;
    DSpinBox *m_sysSpinBox = Q_NULLPTR;
    DTipLabel *m_dataSizeTip = Q_NULLPTR;
    DTipLabel *m_otherSizeTip = Q_NULLPTR;
    QButtonGroup *m_modeGroup = Q_NULLPTR;
    int m_sysMinSize;
    DLabel *m_labelTip;
    DLabel *m_sysInfo;
    DLabel *m_dataInfo;
    DLabel *m_otherInfo;
    HelpLabel *m_helpLabel = Q_NULLPTR;
    OtherPartitionDialog *m_otherPartitionDialog = Q_NULLPTR;
    DLabel *m_installLabel;
    HelpRadioButton *m_btnDefault =  Q_NULLPTR;
    DTipLabel *m_modeTip_1 = Q_NULLPTR;
    HelpRadioButton *m_btnEncrypt = Q_NULLPTR;
    DTipLabel *m_modeTip_2 = Q_NULLPTR;
    HelpRadioButton *m_btnExtand = Q_NULLPTR;
    DTipLabel *m_modeTip_3 = Q_NULLPTR;
    HelpRadioButton *m_btnKeepData = Q_NULLPTR;
    DTipLabel *m_modeTip_4 = Q_NULLPTR;
    DTipLabel *m_modeTip_5 = Q_NULLPTR;

    SizeEditDialog *m_sizeEditDialog = Q_NULLPTR;
    SDeviceInfo m_dev;
    QString m_fullEncryptionPwd = Q_NULLPTR;
    bool m_autoDecrypt = false;

    QWidget *m_contentWidget = Q_NULLPTR;
};

#endif // FULLDISKPARTITIONFRAME_H
