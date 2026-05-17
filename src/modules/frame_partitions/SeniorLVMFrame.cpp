#include "SeniorLVMFrame.h"
#include "TitleWidget.h"
#include "DiskinfoWidget.h"
#include "globals.h"
#include "CommonFunc.h"
#include "SeniorLVMListView.h"
#include "CreateNewPartationDialog.h"
#include "CreateNewLVMDialog.h"
#include "PartitionSeniorManager.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QRegularExpressionValidator>

SeniorLVMFrame::SeniorLVMFrame(QWidget *parent):QWidget (parent)
{
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addWidget(m_titleWidget);

    //列表
    m_lvmCreate = new DLabel(this);
    m_lvmCreate->setFixedHeight(36);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(0);
    btnLayout->addSpacing(CommonFunc::isBigScreen() ? 35 : 25);
    btnLayout->addWidget(m_lvmCreate);
    btnLayout->addStretch();

    m_tableView = new SeniorLVMListView(this);
    m_tableView->setFixedSize(CommonFunc::isBigScreen() ? QSize(980, 475) : QSize(680, 255));
    connect(m_tableView, &SeniorLVMListView::existSelectedItem, this, [=](bool bFlag) {
        m_okBtn->setEnabled(bFlag);
    });
    m_model = new LVMItemModel;
    m_tableView->setModel(m_model);
    QHeaderView *pHeaderView = m_tableView->header();
    pHeaderView->setSectionResizeMode(QHeaderView::Stretch);
    pHeaderView->setSectionResizeMode(0, QHeaderView::Fixed);
    pHeaderView->setSectionResizeMode(1, QHeaderView::Fixed);
    pHeaderView->setSectionResizeMode(2, QHeaderView::Fixed);
    pHeaderView->setSectionResizeMode(3, QHeaderView::Fixed);
    pHeaderView->setSectionResizeMode(4, QHeaderView::Fixed);
    pHeaderView->setDefaultSectionSize(36);

    m_tableView->setColumnWidth(0, CommonFunc::isBigScreen() ? createLvm::kColumnWidth_0 : createLvm::kColumnWidthSmall_0);
    m_tableView->setColumnWidth(1, CommonFunc::isBigScreen() ? createLvm::kColumnWidth_1 : createLvm::kColumnWidthSmall_1);
    m_tableView->setColumnWidth(2, CommonFunc::isBigScreen() ? createLvm::kColumnWidth_2 : createLvm::kColumnWidthSmall_2);
    m_tableView->setColumnWidth(3, CommonFunc::isBigScreen() ? createLvm::kColumnWidth_3 : createLvm::kColumnWidthSmall_3);
    m_tableView->setColumnWidth(4, CommonFunc::isBigScreen() ? createLvm::kColumnWidth_4 : createLvm::kColumnWidthSmall_4);
    QHBoxLayout *viewLayout = new QHBoxLayout;
    viewLayout->setContentsMargins(0, 0, 0, 0);
    viewLayout->addStretch();
    viewLayout->addWidget(m_tableView);
    viewLayout->addStretch();

    QVBoxLayout *listLayout = new QVBoxLayout;
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->addStretch();
    listLayout->addLayout(btnLayout);
    listLayout->addLayout(viewLayout);
    listLayout->addStretch();

    m_tipLabel = new DLabel(this);
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setFixedSize(600, 40);
    m_tipLabel->setWordWrap(true);
    DPalette pe = m_tipLabel->palette();
    pe.setColor(DPalette::WindowText, QColor("#FF7400"));
    m_tipLabel->setBackgroundRole(DPalette::Window);
    m_tipLabel->setPalette(pe);

    QHBoxLayout *tipsLayout = new QHBoxLayout;
    tipsLayout->setContentsMargins(0, 0, 0, 0);
    tipsLayout->addStretch();
    tipsLayout->addWidget(m_tipLabel);
    tipsLayout->addStretch();

    m_group = new DLabel(this);
    m_group->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    m_groupCreate = new DLineEdit(this);
    m_groupCreate->setFixedSize(160, 36);

    m_cancelBtn = new DPushButton(this);
    m_cancelBtn->setFixedSize(130, 36);
    m_cancelBtn->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    connect(m_cancelBtn, &DSuggestButton::clicked, this, &SeniorLVMFrame::back);

    m_okBtn = new DSuggestButton(this);
    m_okBtn->setFixedSize(130, 36);
    m_okBtn->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    m_okBtn->setEnabled(false);
    connect(m_okBtn, &DSuggestButton::clicked, this, [=]{
        QString vgName = m_groupCreate->text();
        if (PartitionSeniorManager::instance()->existVGName(vgName)) {
            this->updateTips(::QObject::tr("The name already exists"));
            return;
        }

        QRegularExpression exp("^([a-zA-Z0-9_-]{2,20})$");
        
        if (exp.isValid() && !exp.match(vgName).hasMatch()) {
            this->updateTips(::QObject::tr("The VG name must be 2-20 characters long and only contain letters and numbers"));
        } else {
            QList<SPartitionInfo> pvLists = m_tableView->getPartitionInfos();
            qInfo() << "pvLists = " << pvLists.size();
            PartitionSeniorManager::instance()->newVG(vgName, pvLists);
            this->updateTips("");

            Q_EMIT this->createLVM();
        }
    });

    QHBoxLayout *okLayout = new QHBoxLayout;
    okLayout->setContentsMargins(0, 0, 0, 0);
    okLayout->setSpacing(0);
    okLayout->addSpacing(30);
    okLayout->addWidget(m_group);
    okLayout->addSpacing(20);
    okLayout->addWidget(m_groupCreate);
    okLayout->addStretch();
    okLayout->addWidget(m_cancelBtn);
    okLayout->addSpacing(10);
    okLayout->addWidget(m_okBtn);
    okLayout->addSpacing(30);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(titleLayout);
    mainLayout->addLayout(listLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(tipsLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(okLayout);
    this->setLayout(mainLayout);
}

void SeniorLVMFrame::setDevData(const QMap<SDeviceInfo, QList<SPartitionInfo> > &devs)
{
    m_tableView->resetListView();
    m_model->setDatas(devs);
    m_groupCreate->setText(PartitionSeniorManager::instance()->getVGName());
    m_okBtn->setEnabled(false);
    m_tipLabel->setText("");
}

void SeniorLVMFrame::updateTips(const QString &text)
{
    m_tipLabel->setText(text);
}

void SeniorLVMFrame::updateTs()
{
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Advanced"), "");
    m_titleWidget->setTipsLabelProperty(::QObject::tr("You can freely allocate disk space, configure file systems and mount points, etc."), "font-size:16px;font-weight:Medium;");

    m_lvmCreate->setText(::QObject::tr("Create LVM"));

    m_group->setText(::QObject::tr("VG name"));

    m_cancelBtn->setText(::QObject::tr("Cancel", "button"));

    m_okBtn->setText(::QObject::tr("Confirm", "button"));
}
