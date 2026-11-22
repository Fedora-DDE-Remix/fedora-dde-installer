#include "CreateNewSysDialog.h"
#include "globals.h"
#include "structs.h"
#include "PartitionSizeSlider.h"
#include "TitleWidget.h"

#include <DCheckBox>
#include <QScreen>

CreateNewSysDialog::CreateNewSysDialog(const SPartitionInfo &free, QWidget *parent):
    DDialog (parent),
    m_freePartitionInfo(free)
{
    m_titleWidget = new TitleWidget(this);
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Space Allocation"), "font-size:16px;font-weight:Medium;");
    m_titleWidget->setTipsLabelProperty(::QObject::tr("Btrfs root and /home share system space. Set data space above zero only for a separate /home partition."), "font-size:14px;");
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addStretch();
    titleLayout->addWidget(m_titleWidget);
    titleLayout->addStretch();

    QWidget *pInnerWidget = new QWidget(this);
    m_tipsLabel = new DLabel(this);
    m_tipsLabel->setAlignment(Qt::AlignCenter);
    m_tipsLabel->setFixedSize(420, 40);
    m_tipsLabel->setWordWrap(true);

    m_formatPartition = new DCheckBox(::QObject::tr("Format the partition"));
    m_formatPartition->setCheckState(Qt::CheckState::Checked);
    m_formatPartition->setEnabled(false);
//    m_formatPartition->setCheckState(info.isNeedFormat ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    QString formattedTips = ::QObject::tr("This partition will be formatted. Make sure you have backed up all data.");
    updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? formattedTips : "");
    connect(m_formatPartition, &DCheckBox::clicked, this, [=] {
        updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? formattedTips : "");
    });
    QHBoxLayout *formatPartitionHLayout = new QHBoxLayout;
    formatPartitionHLayout->addStretch();
    formatPartitionHLayout->addWidget(m_formatPartition);
    formatPartitionHLayout->addStretch();

    m_systemLabel = new DLabel(this);
    m_systemLabel->setFixedSize(100, 36);
    m_systemLabel->setText(::QObject::tr("System space"));

    m_editSystemSlider = new PartitionSizeSlider(this);
    m_editSystemSlider->setMinimum(0);
    m_editSystemSlider->setValue(0);
    m_editSystemSlider->setMaximum(free.size);
    m_editSystemSlider->setFixedSize(300, 36);
    m_editSystemSlider->setStyleSheet("#partition_size_slider:focus{border:2px solid; border-color:rgb(1, 128, 255); border-radius:5px; padding:2px 4px;}");

    QHBoxLayout *pSystemLayout = new QHBoxLayout;
    pSystemLayout->setSpacing(glayoutSpacing);
    pSystemLayout->addStretch();
    pSystemLayout->addWidget(m_systemLabel, 0, Qt::AlignLeft);
    pSystemLayout->addWidget(m_editSystemSlider, 0, Qt::AlignRight);
    pSystemLayout->addStretch();

    m_dataLabel = new DLabel(this);
    m_dataLabel->setFixedSize(100, 36);
    m_dataLabel->setText(::QObject::tr("Data space"));

    m_editDataSlider = new PartitionSizeSlider(this);
    m_editDataSlider->setMinimum(0);
    m_editDataSlider->setValue(0);
    m_editDataSlider->setMaximum(free.size);
    m_editDataSlider->setFixedSize(300, 36);
    m_editDataSlider->setStyleSheet("#partition_size_slider:focus{border:2px solid; border-color:rgb(1, 128, 255); border-radius:5px; padding:2px 4px;}");
    QHBoxLayout *pDataLayout = new QHBoxLayout;
    pDataLayout->setSpacing(glayoutSpacing);
    pDataLayout->addStretch();
    pDataLayout->addWidget(m_dataLabel, 0, Qt::AlignLeft);
    pDataLayout->addWidget(m_editDataSlider, 0, Qt::AlignRight);
    pDataLayout->addStretch();

    QHBoxLayout *pHLayout = new QHBoxLayout;
    m_cancelBtn = new DPushButton(::QObject::tr("Cancel", "button"), this);
    m_cancelBtn->setFixedSize(200, 36);

    m_okBtn = new DSuggestButton(::QObject::tr("Confirm", "button"), this);
    m_okBtn->setFixedSize(200, 36);
    pHLayout->addStretch();
    pHLayout->addWidget(m_cancelBtn);
    pHLayout->addSpacing(15);
    pHLayout->addWidget(m_okBtn);
    pHLayout->addStretch();

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setContentsMargins(0, 0, 0, 0);
    pVLayout->addLayout(titleLayout);
    pVLayout->addLayout(pSystemLayout);
    pVLayout->addLayout(pDataLayout);
    pVLayout->addSpacing(glayoutSpacing);
    pVLayout->addLayout(formatPartitionHLayout);
    pVLayout->addWidget(m_tipsLabel, Qt::AlignHCenter);
    pVLayout->addLayout(pHLayout);

    QHBoxLayout *pMainLayout = new QHBoxLayout;
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->addStretch();
    pMainLayout->addLayout(pVLayout);
    pMainLayout->addStretch();
    pInnerWidget->setLayout(pMainLayout);

    addContent(pInnerWidget);
    setFixedSize(440, 400);

    this->setOnButtonClickedClose(true);

    initConnection();

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

void CreateNewSysDialog::setSystemSpaceRange(const qint64 &Minimum, const qint64 &Maximum)
{
    m_editSystemSlider->setMinimum(Minimum);
    m_editSystemSlider->setMaximum(Maximum);
    m_editSystemSlider->setValue(Maximum);
}

void CreateNewSysDialog::setDataSpaceRange(const qint64 &Minimum, const qint64 &Maximum)
{
    m_editDataSlider->setMinimum(Minimum);
    m_editDataSlider->setMaximum(Maximum);
    m_editDataSlider->setValue(Minimum);
}

void CreateNewSysDialog::initConnection()
{
    connect(m_editSystemSlider, &PartitionSizeSlider::valueChanged, this, [&](qint64 value) {
        if (value + m_editDataSlider->value() >= m_freePartitionInfo.size) {
            m_editSystemSlider->blockSignals(true);
            m_editSystemSlider->setValue(m_freePartitionInfo.size - m_editDataSlider->value());
            m_editSystemSlider->blockSignals(false);
        }
    });

//    connect(m_editSystemSlider, &PartitionSizeSlider::setNextEnable, this, [=](bool flag){
//        m_okBtn->setEnabled(flag);
//    });

    connect(m_editDataSlider, &PartitionSizeSlider::valueChanged, this, [&](qint64 value) {
        if (value + m_editSystemSlider->value() >= m_freePartitionInfo.size) {
            m_editDataSlider->blockSignals(true);
            m_editDataSlider->setValue(m_freePartitionInfo.size - m_editSystemSlider->value());
            m_editDataSlider->blockSignals(false);
        }
    });

//    connect(m_editDataSlider, &PartitionSizeSlider::setNextEnable, this, [=](bool flag){
//        m_okBtn->setEnabled(flag);
//    });

    connect(m_cancelBtn, &DPushButton::clicked, this, [&] {
        close();
    });

    connect(m_okBtn, &DSuggestButton::clicked, this, &DDialog::accept);
}

qint64 CreateNewSysDialog::getSystemSpaceSize()
{
    return m_editSystemSlider->value();
}

qint64 CreateNewSysDialog::getDataSpaceSize()
{
    return m_editDataSlider->value();
}

void CreateNewSysDialog::updateErrTips(const QString &err)
{
    m_tipsLabel->setStyleSheet(QString("QLabel{color:%1;}").arg("#FF7400"));
    m_tipsLabel->setText(err);
}

bool CreateNewSysDialog::getFormatPartitionCheckStatus()
{
    return m_formatPartition->isChecked();
}

