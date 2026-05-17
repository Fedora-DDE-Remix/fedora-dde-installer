//
// Created by uos on 2022/3/18.
//

#include "EditionItemFrame.h"
#include "CommonFunc.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <DFontSizeManager>
#include <QDebug>

EditionItemFrame::EditionItemFrame(QWidget *parent)
    : DFrame(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setAttribute(Qt::WA_Hover,true);
    setObjectName("EditionItemFrame");
    setBackgroundRole(Dtk::Gui::DPalette::Window);//ItemBackground);
    setLineWidth(1);
    initUI();
}

void EditionItemFrame::setIconPixmap(const QString &iconRes, const QSize &iconSize)
{
    m_icon->setPixmap(CommonFunc::hidpiPixmap(iconRes, iconSize));
}

void EditionItemFrame::setItemType(ItemFrameType type)
{
    m_type = type;
}

ItemFrameType EditionItemFrame::getItemType()
{
    return m_type;
}

void EditionItemFrame::setCheckState(bool checked)
{
    m_bChecked = checked;
    if (m_bChecked) {
        m_checkIcon->setVisible(true);
        m_checkIcon->setPixmap(CommonFunc::hidpiPixmap(":/images/radio_button.svg", QSize(20, 20)));
        m_bChecked = true;
    } else {
        m_checkIcon->setPixmap(QPixmap());
        m_bChecked = false;
    }
}

void EditionItemFrame::setTitleText(const QString &text)
{
    m_title->setText(text);
}

void EditionItemFrame::setDescriptionText(const QString &text)
{
    m_description->setText(text);
}

void EditionItemFrame::initUI()
{
    QVBoxLayout *centralLayout = new QVBoxLayout(this);
    auto hBoxLayout = new QHBoxLayout;

    hBoxLayout->addStretch();

    m_checkIcon = new DLabel;
    m_checkIcon->setVisible(true);
    m_checkIcon->setFixedSize(QSize(20, 20));
    hBoxLayout->addWidget(m_checkIcon);

    centralLayout->addLayout(hBoxLayout);

    m_icon = new DLabel;
    m_icon->setAlignment(Qt::AlignCenter);

    centralLayout->addWidget(m_icon, 0, Qt::AlignHCenter);
    centralLayout->addSpacing(10);

    QVBoxLayout *vLayout = new QVBoxLayout();
    m_title = new DLabel;
    m_title->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    m_title->setWordWrap(true);
    m_title->setFixedHeight(60);
    DFontSizeManager::instance()->bind(m_title, DFontSizeManager::T4, 450);

    m_description = new DLabel;
    m_description->hide();
    m_description->setAlignment(Qt::AlignCenter);
    m_description->setWordWrap(true);
    QFont descriptionFont = m_description->font();
    descriptionFont.setPixelSize(12);
    m_description->setFont(descriptionFont);

    vLayout->addWidget(m_title);
    vLayout->addWidget(m_description);

    centralLayout->addLayout(vLayout);
    centralLayout->setSpacing(0);

    this->setLayout(centralLayout);
}

bool EditionItemFrame::event(QEvent *e)
{
    if (e->type() == QEvent::HoverEnter) {
        setBackgroundRole(DPalette::ObviousBackground);
    } else if (e->type() == QEvent::HoverLeave) {
        setBackgroundRole(DPalette::ItemBackground);
    }

    return QFrame::event(e);
}
