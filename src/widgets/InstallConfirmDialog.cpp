#include "InstallConfirmDialog.h"
#include "globals.h"
#include "CommonFunc.h"
#include "TitleWidget.h"
#include "HelpLabel.h"
#include "HelpDialog.h"
#include <DFrame>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QScrollBar>
#include <DScrollArea>

#include <iostream>

InstallConfirmDialog::InstallConfirmDialog(QWidget *parent):
    DDialog (parent)
{
    const int dialogWidth = CommonFunc::getShowSize().width() - 124;
    const int dialogHeight = CommonFunc::getShowSize().height() - 38;
    const int scrollAreaWidth = CommonFunc::isBigScreen() ? CommonFunc::getShowSize().width() - 424 : CommonFunc::getShowSize().width() - 180;
    const int scrollAreaHeight = CommonFunc::isBigScreen() ? CommonFunc::getShowSize().height() - 358 : CommonFunc::getShowSize().height() - 258;
    const int scrollAreaWidgetWidth = CommonFunc::isBigScreen() ? CommonFunc::getShowSize().width() - 424 : CommonFunc::getShowSize().width() - 180;
    const int scrollAreaWidgetHeight = CommonFunc::isBigScreen() ? CommonFunc::getShowSize().height() - 340 : CommonFunc::getShowSize().height() - 258;

    this->setFixedSize(dialogWidth, dialogHeight);

    // 标题栏
    TitleWidget *titleWidget = new TitleWidget(this);
    titleWidget->setTitleLabelProperty(::QObject::tr("Ready to Install"), "");
    titleWidget->setTipsLabelProperty(::QObject::tr("Make a backup of your important data and then continue"), "font-size:16px;font-weight:Medium;", true);

    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addStretch();
    titleLayout->addWidget(titleWidget);
    titleLayout->addStretch();

    QWidget *titleMainWidget = new QWidget;
    titleMainWidget->setLayout(titleLayout);

    DWidget *MainWidget = new DWidget(this);
    MainWidget->setFixedSize(dialogWidth, dialogHeight);
    //  设置Dialog背景白色
    QPalette paletteMainWidget(MainWidget->palette());
    paletteMainWidget.setColor(QPalette::Window, QColor(247, 247, 247));
    MainWidget->setAutoFillBackground(true);
    MainWidget->setPalette(paletteMainWidget);
    //  用DLabel来显示协议文案
    m_operatorInfo = new DLabel;
    m_operatorInfo->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_operatorInfo->setWordWrap(true);
    QPalette paletteLicenseLabel;
    paletteLicenseLabel.setColor(QPalette::WindowText, QColor("#006EFF"));
    m_operatorInfo->setPalette(paletteLicenseLabel);

    //  用DBlurEffectWidget来实现DLabel半透明模糊效果
    DBlurEffectWidget *blurEffectLicenseWidget = new DBlurEffectWidget;
    QHBoxLayout *operatorsInfoLayout = new QHBoxLayout;
    operatorsInfoLayout->setContentsMargins(15, 10, 15, 0);
    operatorsInfoLayout->addWidget(m_operatorInfo);
    blurEffectLicenseWidget->setLayout(operatorsInfoLayout);
    blurEffectLicenseWidget->resize(m_operatorInfo->size());
    blurEffectLicenseWidget->setBlendMode(DBlurEffectWidget::BehindWindowBlend);

    QHBoxLayout *operatorInfoLayout = new QHBoxLayout;
    operatorInfoLayout->setContentsMargins(0, 0, 0, 0);
    operatorInfoLayout->setSpacing(0);
    operatorInfoLayout->addWidget(blurEffectLicenseWidget);
    QWidget *operatorInfoWidget = new QWidget;
    operatorInfoWidget->setLayout(operatorInfoLayout);

    //  设置滚动区域
    DScrollArea * scrollArea = new DScrollArea;
    scrollArea->setWidget(operatorInfoWidget);
    scrollArea->setFixedSize(scrollAreaWidth, scrollAreaHeight);
    scrollArea->setWidgetResizable(true);
    scrollArea->setLineWidth(0);
    scrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    scrollArea->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    scrollArea->horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

    QHBoxLayout *boxLayout = new QHBoxLayout;
    boxLayout->setContentsMargins(0, 0, 0, 0);
    boxLayout->setSpacing(0);
    boxLayout->addStretch();
    boxLayout->addWidget(scrollArea);
    boxLayout->addStretch();

    //将滚动区域设置为半透明模糊效果加上圆角
    DBlurEffectWidget *scrollAreaWidget = new DBlurEffectWidget;
    scrollAreaWidget->setFixedSize(scrollAreaWidgetWidth, scrollAreaWidgetHeight);

    //设置scrollAreaWidget背景灰白色
    QPalette palettescrollAreaWidget(scrollAreaWidget->palette());
    palettescrollAreaWidget.setColor(QPalette::Window, QColor(222, 222, 222, 0));
    scrollAreaWidget->setAutoFillBackground(true);
    scrollAreaWidget->setPalette(palettescrollAreaWidget);
    scrollAreaWidget->setLayout(boxLayout);
    scrollAreaWidget->setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    scrollAreaWidget->setBlurRectXRadius(10);
    scrollAreaWidget->setBlurRectYRadius(10);
    scrollAreaWidget->lower();

    QHBoxLayout *scrollAreaLayout = new QHBoxLayout;
    scrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    scrollAreaLayout->setSpacing(0);
    scrollAreaLayout->addStretch();
    scrollAreaLayout->addWidget(scrollAreaWidget);
    scrollAreaLayout->addStretch();
    QWidget *scrollAreaLayoutWidget = new QWidget;
    scrollAreaLayoutWidget->setLayout(scrollAreaLayout);

    m_helpDialog = new BackupTipDialog;
    m_helpDialog->setVisible(false);
    m_helpDialog->updateTs();

    m_helpLabel = new HelpLabel;
    m_isInitRecovery = new DCheckBox(::QObject::tr("Create initial backup"));
    QHBoxLayout *isInitRecoveryHboxLayout = new QHBoxLayout;
    isInitRecoveryHboxLayout->addStretch();
    isInitRecoveryHboxLayout->addWidget(m_isInitRecovery);
    isInitRecoveryHboxLayout->addSpacing(10);
    isInitRecoveryHboxLayout->addWidget(m_helpLabel);
    isInitRecoveryHboxLayout->addStretch();
    QWidget *isInitRecoveryWidget = new QWidget;
    isInitRecoveryWidget->setLayout(isInitRecoveryHboxLayout);
    connect(m_helpLabel, &HelpLabel::hoverEnter, [=]{
        m_helpDialog->showCenter(m_helpLabel);
    });
    connect(m_helpLabel, &HelpLabel::hoverLeave, [=]{
        m_helpDialog->hide();
    });

    //开始安装按钮
    m_startInstallBtn = new DPushButton(::QObject::tr("Start installation"));
    m_startInstallBtn->setStyleSheet("color: #0180FF;");
    m_startInstallBtn->setFixedSize(202, 36);
    QHBoxLayout *startInstallBtnHboxLayout = new QHBoxLayout;
    startInstallBtnHboxLayout->addStretch();
    startInstallBtnHboxLayout->addWidget(m_startInstallBtn);
    startInstallBtnHboxLayout->addStretch();
    QWidget *startInstallBtn = new QWidget;
    startInstallBtn->setLayout(startInstallBtnHboxLayout);

    //主布局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleMainWidget);
    mainLayout->addStretch();
    mainLayout->addWidget(scrollAreaLayoutWidget);
    mainLayout->addStretch();
    mainLayout->addWidget(isInitRecoveryWidget);
    mainLayout->addWidget(startInstallBtn);
    mainLayout->addSpacing(0);
    MainWidget->setLayout(mainLayout);

    connect(m_startInstallBtn, &DPushButton::clicked, this, [=] {
        CommonFunc::setSettingsValue("DI_IS_INIT_RECOVERY", m_isInitRecovery->isChecked());
        //ghost安装在点击准备安装后写DI_HOOKS_LIST，防止返回用其他方式安装后DI_HOOKS_LIST值被篡改
        if (CommonFunc::getSettingsString("DI_PARTITION_TYPE") == QString::number(GhostMode)) {
            QString hooksdir = CommonFunc::getSystemEnv("DI_INSTALL_TOOLS_DIR_ENV") + "/hooks/";
            CommonFunc::setSettingsValue("DI_HOOKS_LIST", QString("%1ghost;%2cleanup").arg(hooksdir).arg(hooksdir));
        }
        this->accept();
    });
}


InstallConfirmDialog::~InstallConfirmDialog()
{

}

void InstallConfirmDialog::setOperatorInfo(const QString &info)
{
    if (info.isEmpty()) {
        m_operatorInfo->setText(::QObject::tr("Format the disk %1").arg(CommonFunc::getSettingsStringList("DI_DEVICE_LIST").join(";")));
    } else {
        std::cout << "OperatorInfo: \n" << info.toStdString() << std::endl;
        m_operatorInfo->setText(info);
    }
}

void InstallConfirmDialog::setInitRecovery(bool isInitRecovery)
{
    m_isInitRecovery->setVisible(isInitRecovery);
    m_helpLabel->setVisible(isInitRecovery);
    m_isInitRecovery->setChecked(isInitRecovery);
}
