#include "CreateSwapDialog.h"
#include "TitleWidget.h"
#include "CommonFunc.h"
#include "globals.h"

#include <QHBoxLayout>

CreateSwapDialog::CreateSwapDialog(QWidget *parent):
    DDialog(parent)
{
    m_titleWidget = new TitleWidget(this);
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Create a swap partition"), "font-size:16px;font-weight:Medium;");

    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addStretch();
    titleLayout->addWidget(m_titleWidget);
    titleLayout->addStretch();

    m_contentLabel = new DLabel(this);
    m_contentLabel->setTextFormat(Qt::AutoText);
    m_contentLabel->setFixedWidth(390);
    m_contentLabel->setScaledContents(true);
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setText(QString(::QObject::tr("To improve system performance, it is recommended to create a %1 GB swap partition.")).arg(CommonFunc::mToG(CommonFunc::getSwapSize())));
    QHBoxLayout *pHContentLayout = new QHBoxLayout;
    pHContentLayout->addSpacing(10);
    pHContentLayout->addWidget(m_contentLabel, 0, Qt::AlignHCenter|Qt::AlignTop);

    QWidget *pInnerWidget = new QWidget(this);

    m_cancelBtn = new DPushButton(::QObject::tr("Do not create"), this);
    m_cancelBtn->setFixedSize(200, 36);
    connect(m_cancelBtn, &DPushButton::clicked, this, &DDialog::close);

    m_okBtn = new DSuggestButton(::QObject::tr("OK", "button"), this);
    m_okBtn->setFixedSize(200, 36);
    connect(m_okBtn, &DPushButton::clicked, this, &DDialog::accept);

    QHBoxLayout *pHLayout = new QHBoxLayout;
    pHLayout->addStretch();
    pHLayout->addWidget(m_cancelBtn);
    pHLayout->addSpacing(15);
    pHLayout->addWidget(m_okBtn);
    pHLayout->addStretch();

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setContentsMargins(0, 0, 0, 0);
    pVLayout->addLayout(titleLayout);
    pVLayout->addSpacing(10);
    pVLayout->addLayout(pHContentLayout);
    pVLayout->addStretch();
    pVLayout->addLayout(pHLayout);

    QHBoxLayout *pMainLayout = new QHBoxLayout;
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->addStretch();
    pMainLayout->addLayout(pVLayout);
    pMainLayout->addStretch();
    pInnerWidget->setLayout(pMainLayout);

    addContent(pInnerWidget);
    setFixedSize(440, 280);
}
