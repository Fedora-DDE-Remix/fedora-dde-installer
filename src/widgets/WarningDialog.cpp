#include "WarningDialog.h"

#include <QPainterPath>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPainter>
#include <QScreen>

const int warningDialogWidth = 380;
const int warningDialogHeight = 156;

WarningDialog::WarningDialog(QWidget *parent) : DDialog(parent)
{
    setAttribute(Qt::WA_ShowModal, true);
    initUI();
    initConnections();

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

void WarningDialog::setCommentText(const QString &text)
{
    if (m_commentLabel != nullptr) {
        m_commentLabel->setText(text);
    }
}

void WarningDialog::setBtnText(const QString &text)
{
    if (m_enterButton != nullptr) {
        m_enterButton->setText(text);
    }
}

void WarningDialog::setDialogIcon(const QIcon &icon)
{
    setIcon(icon);
}

void WarningDialog::showEvent(QShowEvent *event)
{
    this->clearFocus();
    return QWidget::showEvent(event);
}

void WarningDialog::initConnections()
{
    connect(this, &DDialog::closed, this, &WarningDialog::closeBtnSignal);
    connect(m_enterButton, &QPushButton::clicked, this, &WarningDialog::enterBtnSignal);
}

void WarningDialog::initUI()
{
    m_commentLabel = new QLabel(this);
    m_commentLabel->setFocusPolicy(Qt::NoFocus);
    m_commentLabel->setStyleSheet("QLabel{color:rgba(0, 0, 0, 0.6);}");
    m_commentLabel->setFixedWidth(warningDialogWidth - 40);
    m_commentLabel->setWordWrap(true);
    m_commentLabel->setAlignment(Qt::AlignCenter);

    m_enterButton = new QPushButton(this);
    m_enterButton->setFocusPolicy(Qt::NoFocus);
    m_enterButton->setFixedHeight(36);
    m_enterButton->setStyleSheet("QPushButton{ color:#414D68; border:1px solid; border-color:rgba(0, 0, 0, 0.03); border-radius:8px; background-color:rgb(230, 230, 230); } \
                            QPushButton:hover{ color:#414D68; border:1px solid; border-color:rgba(0, 0, 0, 0.03); border-radius:8px; background-color:rgb(205, 205, 205); } \
                            QPushButton:pressed{ color:#0081FF; border:1px solid; border-color:rgba(0, 0, 0, 0.03); border-radius:8px; background-color:rgb(188, 196, 208); }");

    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(5, 5, 5, 5);
    vLayout->setSpacing(10);
    vLayout->addStretch();
    vLayout->addWidget(m_commentLabel, 0, Qt::AlignCenter);
    vLayout->addStretch();
    vLayout->addWidget(m_enterButton, 0, Qt::AlignBottom);

    QWidget *pInnerWidget = new QWidget;
    pInnerWidget->setLayout(vLayout);
    addContent(pInnerWidget);

    setMaximumWidth(warningDialogWidth);
    setMinimumWidth(warningDialogHeight);
}
