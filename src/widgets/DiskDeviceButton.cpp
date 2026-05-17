#include "DiskDeviceButton.h"
#include "CommonFunc.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QString>
#include <QCoreApplication>
#include <QTime>
#include <DFontSizeManager>

DiskDeviceButton::DiskDeviceButton(QWidget *parent):
    QWidget(parent)
{
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(CommonFunc::getDiskIconSize());
    m_iconLabel->setPixmap(CommonFunc::loadSvgImage(":/images/drive-harddisk-128px.svg", m_iconLabel->width(), m_iconLabel->height()));
    m_iconLabel->installEventFilter(this);
    QHBoxLayout *iconLayout = new QHBoxLayout;
    iconLayout->setSpacing(0);
    iconLayout->addStretch();
    iconLayout->addWidget(m_iconLabel);
    iconLayout->addStretch();

    m_deviceInfoLabel = new DTipLabel("", this);
    m_deviceInfoLabel->setAlignment(Qt::AlignCenter);
    m_deviceNameLabel = new QLabel(this);
    m_deviceNameLabel->setAlignment(Qt::AlignCenter);
    QVBoxLayout *deviceInfoLayout = new QVBoxLayout;
    deviceInfoLayout->setSpacing(0);
    deviceInfoLayout->setContentsMargins(0, 0, 0, 0);
    deviceInfoLayout->addWidget(m_deviceInfoLabel);
    deviceInfoLayout->addWidget(m_deviceNameLabel);
    DFontSizeManager::instance()->bind(m_deviceInfoLabel, DFontSizeManager::T6);
    DFontSizeManager::instance()->bind(m_deviceNameLabel, DFontSizeManager::T5, QFont::DemiBold);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addStretch();
    mainLayout->addLayout(iconLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(deviceInfoLayout);
    mainLayout->addStretch();
    this->setLayout(mainLayout);
}

void DiskDeviceButton::setDeviceInfo(const SDeviceInfo &info)
{
    m_deviceNameLabel->setText(info.name);
    m_deviceInfoLabel->setText(info.device);
    m_deviceInfo = info;
}

bool DiskDeviceButton::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_iconLabel) {
        switch (event->type()) {
            case QEvent::MouseButtonRelease: {
                if (!m_isSelect) {
                    emit clicked(m_deviceInfo);
                    m_isSelect = true;
                }
            } break;
            case QEvent::Enter: setCursor(QCursor(Qt::PointingHandCursor)); break;
            case QEvent::Leave: setCursor(QCursor(Qt::ArrowCursor)); break;
            default: break;
        }
    }

    return QWidget::eventFilter(watched, event);
}





