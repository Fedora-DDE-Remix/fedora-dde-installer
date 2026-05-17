#include "TitleWidget.h"
#include "CommonFunc.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <DFontSizeManager>

TitleWidget::TitleWidget(QWidget *parent)
    : QWidget (parent)
{
    // 标题栏
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(m_titleLabel);

    QHBoxLayout *tipLayout = new QHBoxLayout;
    if (CommonFunc::isBigScreen()) {
        tipLayout->setContentsMargins(40, 0, 40, 0);
    } else {
        tipLayout->setContentsMargins(0, 0, 0, 0);
    }
   
    m_tipsLabel = new QLabel(this);
    m_tipsLabel->setAlignment(Qt::AlignHCenter);
    m_tipsLabel->setWordWrap(true);
    tipLayout->addWidget(m_tipsLabel);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(titleLayout);
    mainLayout->addLayout(tipLayout);
    this->setLayout(mainLayout);
}

void TitleWidget::setTitleLabelProperty(const QString &strTitle, const QString &strStyleSheet)
{
    if (!strTitle.isEmpty()) {
        m_titleLabel->setText(strTitle);
    }

    if (!strStyleSheet.isEmpty()) {
        m_titleLabel->setStyleSheet(strStyleSheet);
    } else {
        DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T3, 450);
    }

}


void TitleWidget::setTipsLabelProperty(const QString &strTips, const QString &strStyleSheet, bool isUseDefaultWidth)
{
    m_tipsLabel->setText(strTips);

    if (!strStyleSheet.isEmpty()) {
        m_tipsLabel->setStyleSheet(strStyleSheet);
    }

    if (!isUseDefaultWidth) {
        setTipsLabelWidth(this->parentWidget()->width() - 40);
    }
}

void TitleWidget::setTipsLabelWidth(int width)
{
    m_tipsLabel->setFixedWidth(width);
}

int TitleWidget::getTotalHeight()
{
    return m_titleLabel->height() + m_tipsLabel->height();
}

void TitleWidget::setTipsLabelVisible(bool isVisible)
{
    m_tipsLabel->setVisible(isVisible);
}

void TitleWidget::setTipsLabelHide()
{
    m_tipsLabel->hide();
}
