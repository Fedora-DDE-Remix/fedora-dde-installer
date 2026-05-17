#include "SizeEditDialog.h"
#include "globals.h"
#include "structs.h"
#include "PartitionSizeSlider.h"
#include "CommonFunc.h"
#include "TitleWidget.h"
#include <DPaletteHelper>
#include <DFrame>
#include <DBackgroundGroup>
#include <DTipLabel>
#include <QtMath>
#include <QScreen>

MultidiskSizeDialog::MultidiskSizeDialog(QWidget *parent):DDialog (parent)
{
    setFixedWidth(440);
    setTitle(tr("Resize the system partition"));

    QWidget *content = new QWidget;
    QVBoxLayout *contentLayout = new QVBoxLayout;
    content->setLayout(contentLayout);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    DBackgroundGroup *bgGroup = new DBackgroundGroup(groupLayout);
    bgGroup->setItemMargins(QMargins(0, 0, 0, 0));
    bgGroup->setItemSpacing(1);

    QPalette palette = bgGroup->palette();
    palette.setColor(QPalette::Window, QColor("#dfdfdf"));
    bgGroup->setPalette(palette);
    bgGroup->setBackgroundRole(QPalette::Window);
    //
    QWidget *sysWidget = new QWidget;
    sysWidget->setFixedHeight(48);
    QHBoxLayout *sysLayout = new QHBoxLayout;
    sysLayout->setContentsMargins(10, 0, 10, 0);
    sysLayout->setSpacing(0);
    sysWidget->setLayout(sysLayout);
    DLabel *labelName = new DLabel;
    labelName->setText(tr("System"));
    m_sysSizeSlider = new DSlider(Qt::Horizontal);
    m_sysSizeSlider->setFixedWidth(178);
    m_sysSizeEdit = new DLineEdit;
    m_sysSizeEdit->setClearButtonEnabled(false);
    m_sysSizeEdit->setFixedWidth(80);
    DTipLabel *sizeTip = new DTipLabel;
    sizeTip->setText("GiB");
    sysLayout->addWidget(labelName, 0, Qt::AlignVCenter);
    sysLayout->addStretch();
    sysLayout->addWidget(m_sysSizeSlider, 0, Qt::AlignVCenter);
    sysLayout->addSpacing(10);
    sysLayout->addWidget(m_sysSizeEdit, 0, Qt::AlignVCenter);
    sysLayout->addSpacing(10);
    sysLayout->addWidget(sizeTip, 0, Qt::AlignVCenter);
    //
    QWidget *dataWidget = new QWidget;
    dataWidget->setFixedHeight(48);
    QHBoxLayout *dataLayout = new QHBoxLayout;
    dataLayout->setContentsMargins(10, 0, 10, 0);
    dataLayout->setSpacing(0);
    dataWidget->setLayout(dataLayout);
    DLabel *labelData = new DLabel;
    labelData->setText(tr("Data"));
    m_labelDataSize = new DLabel;
    DTipLabel *sizeTipData = new DTipLabel;
    sizeTipData->setText("GiB");
    dataLayout->addWidget(labelData, 0, Qt::AlignLeft);
    dataLayout->addStretch();
    dataLayout->addWidget(m_labelDataSize);
    dataLayout->addSpacing(10);
    dataLayout->addWidget(sizeTipData, 0, Qt::AlignRight);
    //
    QWidget *otherWidget = new QWidget;
    otherWidget->setFixedHeight(48);
    QHBoxLayout *otherLayout = new QHBoxLayout;
    otherLayout->setContentsMargins(10, 0, 10, 0);
    otherLayout->setSpacing(0);
    otherWidget->setLayout(otherLayout);
    DLabel *labelOther = new DLabel;
    labelOther->setText(tr("Other partitions"));
    m_labelOtherSize = new DLabel;
    DTipLabel *sizeTipOther = new DTipLabel;
    sizeTipOther->setText("GiB");
    otherLayout->addWidget(labelOther);
    otherLayout->addStretch();
    otherLayout->addWidget(m_labelOtherSize);
    otherLayout->addSpacing(10);
    otherLayout->addWidget(sizeTipOther);

    groupLayout->addWidget(sysWidget);
    groupLayout->addWidget(dataWidget);
    groupLayout->addWidget(otherWidget);
    contentLayout->addWidget(bgGroup);
    contentLayout->addStretch();
    addSpacing(20);
    addContent(content);
    addSpacing(20);
    addButton(::QObject::tr("Cancel"));
    addButton(::QObject::tr("Confirm"), true, DDialog::ButtonRecommend);
    initConnection();

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

void MultidiskSizeDialog::initDiskSize(int total, int sysMin, int sys, int other)
{
    qInfo() << Q_FUNC_INFO << total << sysMin << sys << other;
    m_total = total;
    m_other = other;
    m_dataMin = (int)CommonFunc::mToG(CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG"));
    m_sysSizeSlider->setMinimum(sysMin);
    m_sysSizeSlider->setMaximum(m_total - other - m_dataMin);
    m_sysSizeSlider->setValue(sys);
    m_sysSizeEdit->setText(QString::number(sys));
    m_labelDataSize->setText(QString::number(m_total - sys - other));
    m_labelOtherSize->setText(QString::number(other));
}

int MultidiskSizeDialog::getSysDiskSize()
{
    return m_sysSizeSlider->value();
}

void MultidiskSizeDialog::initConnection()
{
    connect(m_sysSizeSlider, &DSlider::valueChanged, [this](int value){
        m_sysSizeEdit->setText(QString::number(value));
        m_labelDataSize->setText(QString::number(m_total - value - m_other));
        m_labelOtherSize->setText(QString::number(m_other));
    });
    connect(m_sysSizeEdit, &DLineEdit::textChanged, [this](const QString &value){
        m_sysSizeSlider->setValue(value.toInt());
    });
}

SizeEditDialog::SizeEditDialog(QWidget *parent): DDialog (parent)
{
    setTitle(::QObject::tr("Disk Space"));

    m_DataLabel = new DLabel(this);
    m_DataLabel->setText(::QObject::tr("Data space"));

    m_DataShowLabel = new DLabel(this);
    m_DataShowLabel->setFixedWidth(340);

    QHBoxLayout *pDataShowLayout = new QHBoxLayout;
    pDataShowLayout->setContentsMargins(0, 0, 0, 0);
    pDataShowLayout->addWidget(m_DataLabel, 0, Qt::AlignLeft);
    pDataShowLayout->addWidget(m_DataShowLabel, 0, Qt::AlignLeft);

    m_SystemLabel = new DLabel(this);
    m_SystemLabel->setText(::QObject::tr("System Space"));

    m_editSlider = new PartitionSizeSlider(this);
    m_editSlider->setFixedWidth(340);
    m_editSlider->setStyleSheet("#partition_size_slider:focus{border:2px solid; border-color:rgb(1, 128, 255); border-radius:5px; padding:2px 4px;}");
    QHBoxLayout *pSliderLayout = new QHBoxLayout;
    pSliderLayout->setContentsMargins(0, 0, 0, 0);
    pSliderLayout->addWidget(m_SystemLabel, 0, Qt::AlignLeft);
    pSliderLayout->addWidget(m_editSlider, 0, Qt::AlignRight);

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setContentsMargins(0, 0, 0, 0);
    pVLayout->setSpacing(20);
    pVLayout->addLayout(pDataShowLayout);
    pVLayout->addLayout(pSliderLayout);

    QWidget *pInnerWidget = new QWidget(this);
    pInnerWidget->setLayout(pVLayout);
    addSpacing(20);
    addContent(pInnerWidget);
    addButton(::QObject::tr("Cancel", "button"));
    addButton(::QObject::tr("Confirm", "button"), true, DDialog::ButtonRecommend);
    getButton(1)->setEnabled(false);
    setFixedWidth(440);

    initConnection();
}

qint64 SizeEditDialog::getSize()
{
    return m_editSlider->value();
}

void SizeEditDialog::setValueRange(qint64 minSize, qint64 totalSize, qint64 minDataSize)
{
    m_totalSize = totalSize;
    //设置最大值需要在最小值之前，不然会有bug
    m_editSlider->setMaximum(totalSize - minDataSize);
    m_editSlider->setMinimum(minSize);
}

void SizeEditDialog::setDefaultSize(qint64 nSize)
{
    m_editSlider->setValue(nSize);
}

void SizeEditDialog::setDataSpaceSize(qint64 nSize)
{
    double showSize = nSize;
    QString unit = CommonFunc::showSizeUnit(showSize);
    m_DataShowLabel->setText(QString("%1%2").arg(QString::number(showSize), unit));
}

void SizeEditDialog::updateTs()
{
    setTitle(::QObject::tr("Disk Space"));
    m_DataLabel->setText(::QObject::tr("Data space"));
    m_SystemLabel->setText(::QObject::tr("System space"));
    getButton(0)->setText(::QObject::tr("Cancel", "button"));
    getButton(1)->setText(::QObject::tr("Confirm", "button"));
}

void SizeEditDialog::reSetValues()
{
    m_editSlider->setValue(m_editSlider->minimum());
}

void SizeEditDialog::initConnection()
{
    connect(m_editSlider, &PartitionSizeSlider::valueChanged, this, [&](qint64 value) {
        this->setDataSpaceSize(m_totalSize - value);
    });

//    connect(m_editSlider, &PartitionSizeSlider::setNextEnable, [=](bool enabled){
//        getButton(1)->setEnabled(enabled);
//    });
}
