#ifndef PartitionNavFrame_H
#define PartitionNavFrame_H

/*定义导航按钮objectname*/
#ifndef FULLDISK_PARTTION_BTN
#define FULLDISK_PARTTION_BTN QStringLiteral("FullDiskPartitionBnt")
#endif

#ifndef CUSTOM_PARTTION_BTN
#define CUSTOM_PARTTION_BTN QStringLiteral("CustomPartitionBnt")
#endif

#ifndef SENIOR_PARTTION_BTN
#define SENIOR_PARTTION_BTN QStringLiteral("SeniorPartitionBnt")
#endif

#ifndef GHOST_PARTTION_BTN
#define GHOST_PARTTION_BTN QStringLiteral("GhostPartitionBnt")
#endif

#include "FramePluginInterface.h"
#include "PartitionFrameBase.h"
#include <QStackedWidget>
#include <DTipLabel>
#include <DButtonBox>
#include <DSpinner>
#include <DCheckBox>

DWIDGET_USE_NAMESPACE

namespace partitionNavFrame {
    const int kIconWidth = 56;
    const int kIconHeight = 56;

    const int kOtherIconWidth = 36;
    const int kOtherIconHeight = 36;
}

class QPushButton;
class QLabel;
class OptionButton;
class TitleWidget;
class SelectFileWidget;

typedef struct {
    bool result;
    QString error;
    QString warning;
    QString uimgFilePath;
}CheckResult;

class PartitionNavFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
    enum class PartitionType {
        FULLDISK_PART,
        MULTIDISK_PART,
        CUSTOM_PART,
        SENIOR_PART,
    };
public:
    explicit PartitionNavFrame(QWidget *parent = Q_NULLPTR);

public Q_SLOTS:
    void refreshDeviceInfo();
protected:
    void showEvent(QShowEvent *event) override;

private:
    void initConnection();

    void initPartitionFrame();

    void initBtnConnect(const OptionButton &pOptionButton);
    int setItemHeight();

    void updateTs() override;

    void clearGhostSetting();

    void GenerateRecoveryKey();

    CheckResult checkUImg(const QString &uimgFilePath);

private slots:
    void onStartRepairTool();
    QString parseHuaweiType(const QString &type);

private:
    OptionButton *m_fullDiskPartitionBnt = Q_NULLPTR;
    OptionButton *m_seniorPartitionBnt = Q_NULLPTR;
    OptionButton *m_customPartitionBnt = Q_NULLPTR;
    TitleWidget *m_titleWidget = Q_NULLPTR;
    QWidget *m_scrollAreaContentWidget = Q_NULLPTR;
    QWidget *m_mainPage = Q_NULLPTR;
    QWidget *m_imgSelect = Q_NULLPTR;
    DButtonBox *m_modeBox = Q_NULLPTR;
    DLabel *m_imgLabel = Q_NULLPTR;
    DTipLabel *m_ghostDescLabel = Q_NULLPTR;
    DTipLabel *m_ghostTipLabel = Q_NULLPTR;
    DLabel *m_checkStatusImage = Q_NULLPTR;
    DSpinner *m_spinner = Q_NULLPTR;
    DLabel *m_partLabel = Q_NULLPTR;
    SelectFileWidget *m_selectImgWidget = Q_NULLPTR;
    QStackedWidget *m_centerWidget = Q_NULLPTR;
    QMap<PartitionType, PartitionFrameBase*> m_mapPartition;
    CheckResult m_uimgCheckResult;
};

#endif // PartitionNavFrame_H
