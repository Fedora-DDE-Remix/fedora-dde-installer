#include "OptionButton.h"
#include "globals.h"
#include "CommonFunc.h"

#include <QHBoxLayout>
#include <QLabel>
#include <DFontSizeManager>

OptionButton::OptionButton(const int iconOffSet, QWidget *parent):
    DPushButton (parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    setDefault(true);
    m_iconLabel = new QLabel(this);
    QHBoxLayout *iconLayout = new QHBoxLayout;
    iconLayout->addWidget(m_iconLabel);

    m_installTypeLabel = new QLabel(this);
    m_installTypeLabel->setAlignment(Qt::AlignLeading);
    m_installTypeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    m_introductionLabel = new DTipLabel("", this);
    m_introductionLabel->setWordWrap(true);
    m_introductionLabel->setAlignment(Qt::AlignLeading);
    m_introductionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addStretch();
    contentLayout->addWidget(m_installTypeLabel);
    contentLayout->addWidget(m_introductionLabel);
    contentLayout->addStretch();
    m_contentWidget = new QWidget;
    m_contentWidget->setLayout(contentLayout);

    DFontSizeManager::instance()->bind(m_installTypeLabel, DFontSizeManager::T5, QFont::DemiBold);
    DFontSizeManager::instance()->bind(m_introductionLabel, DFontSizeManager::T8);

    QHBoxLayout *navLayout = new QHBoxLayout;
    m_navLabel = new QLabel(this);
    m_navLabel->setFixedSize(12, 12);
    m_navLabel->setPixmap(CommonFunc::loadSvgImage(":/images/nav.svg", m_navLabel->width(), m_navLabel->height()));
    navLayout->addWidget(m_navLabel);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addSpacing(2 * glayoutSpacing + iconOffSet);
    mainLayout->addLayout(iconLayout);
    mainLayout->addWidget(m_contentWidget);
    mainLayout->addSpacing(2 * glayoutSpacing);
    mainLayout->addStretch();
    mainLayout->addLayout(navLayout);
    mainLayout->addSpacing(glayoutSpacing);
    this->setLayout(mainLayout);
    this->setStyleSheet("QPushButton{background-color:rgba(255, 255, 255, 12);}"
                        "QPushButton:hover{background-color:rgba(0, 0, 0, 12);}");
    connect(this, &OptionButton::clicked, this, [=] {
        this->setSelect(*this);
    });
}

void OptionButton::setSelect(const OptionButton &pOptionButton)
{
    Q_EMIT select(pOptionButton);
}

void OptionButton::setInstallTypeText(const QString &strInstallTypeText, const QString &strStyleSheet)
{
    if (!strInstallTypeText.isEmpty()) {
        m_installTypeLabel->setText(strInstallTypeText);
    }

    if (!strStyleSheet.isEmpty()) {
        m_installTypeLabel->setStyleSheet(strStyleSheet);
    }
    m_contentWidget->setFixedWidth(this->width() - 150);
}

void OptionButton::setIntroductionText(const QString &strIntroductionText, const QString &strStyleSheet)
{
    if (!strIntroductionText.isEmpty()) {
        m_introductionLabel->setText(strIntroductionText);
    }

    if (!strStyleSheet.isEmpty()) {
        m_introductionLabel->setStyleSheet(strStyleSheet);
    }

    m_contentWidget->setFixedWidth(this->width() - 150);
}

void OptionButton::setIcon(const QPixmap &icon)
{
    if (!icon.isNull()) {
        m_iconLabel->setFixedSize(icon.size());
        m_iconLabel->setPixmap(icon.scaled(m_iconLabel->width(), m_iconLabel->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void OptionButton::setNavIcon(const QPixmap &icon)
{
    if (!icon.isNull()) {
        m_navLabel->setPixmap(icon.scaled(m_navLabel->width(), m_navLabel->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void OptionButton::setIntroductionVisible(bool isVisibel)
{
    m_introductionLabel->setVisible(isVisibel);
}

void OptionButton::enterEvent(QEnterEvent *e)
{
    DPushButton::enterEvent(e);
    /*分区导航按钮鼠标移上去变成手形状，代表可以点击*/
    this->setCursor(Qt::PointingHandCursor);
}

void OptionButton::leaveEvent(QEvent *e)
{
    DPushButton::leaveEvent(e);
    /*分区导航按钮鼠标移出恢复原来的形状*/
    this->setCursor(Qt::ArrowCursor);
}

