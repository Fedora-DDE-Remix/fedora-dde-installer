#include "FormatDiskTipsDialog.h"
#include "TitleWidget.h"
#include "CommonFunc.h"

#include <DFrame>
#include <QHBoxLayout>
#include <QScreen>

FormatDiskTipsDialog::FormatDiskTipsDialog(const SDeviceInfo &dev, QWidget *parent):
    DDialog (parent)
{
    setFixedWidth(440);
    setTitle(::QObject::tr("Format the disk"));
    QString strMessage = ::QObject::tr("You have a %1 boot loader but a %2 disk, so you should format the disk before installation.")
                                            .arg(CommonFunc::bootloaderToString(CommonFunc::isUEFI()))
                                            .arg(CommonFunc::diskLabelToString(dev.type));

    QHBoxLayout *messageLayout = new QHBoxLayout;
    m_messageLabel = new DLabel(this);
    m_messageLabel->setFixedWidth(360);
    m_messageLabel->setAlignment(Qt::AlignLeft);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setText(strMessage);
    messageLayout->addStretch();
    messageLayout->addWidget(m_messageLabel, 0, Qt::AlignCenter);
    messageLayout->addStretch();
    QWidget *messageWidget = new QWidget;
    messageWidget->setLayout(messageLayout);

    QHBoxLayout *contentLayout = new QHBoxLayout;
    m_contentLabel = new DLabel(this);
    m_contentLabel->setFixedWidth(360);
    m_contentLabel->setAlignment(Qt::AlignLeft);
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setText(QString("%1\n%2")
                            .arg(::QObject::tr("1. Before clicking \"Confirm\", please back up your important data on the disk, which will be formatted later."),
                                 ::QObject::tr("2. If you do not want to format the disk, please click \"Cancel\", exit the installation, and reselect the %1 boot loader to try again.")
                                 .arg(CommonFunc::bootloaderToString(!CommonFunc::isUEFI()))));

    contentLayout->addStretch();
    contentLayout->addWidget(m_contentLabel, 0, Qt::AlignCenter);
    contentLayout->addStretch();
    QWidget *contentWidget = new QWidget;
    contentWidget->setLayout(contentLayout);

    addContent(messageWidget);
    addSpacing(20);
    addContent(contentWidget);
    addSpacing(10);
    addButton(::QObject::tr("Cancel", "button"));
    addButton(::QObject::tr("Confirm", "button"), true, DDialog::ButtonRecommend);

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

