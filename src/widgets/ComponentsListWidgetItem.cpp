#include "ComponentsListWidgetItem.h"

#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

ComponentsListWidgetItem::ComponentsListWidgetItem(QWidget *parent, SelectType selecttype)
    : QWidget(parent), m_selectType(selecttype)
{
    setObjectName("ComponentsListWidgetItem");
    initUI();
}

void ComponentsListWidgetItem::setShowText(const QString &title, const QString &content)
{
    m_titleLabel->setText(title);
    m_contentLabel->setText(content);
}

void ComponentsListWidgetItem::setSelect(bool isselect)
{
    if (m_selectType == SelectType::RadioButtonSelect) {
        m_radioSelect->setChecked(isselect);
    } else if (m_selectType == SelectType::CheckBoxSelect) {
        m_selectBox->setChecked(isselect);
    }
}

bool ComponentsListWidgetItem::isSelected()
{
    if (m_selectType == SelectType::RadioButtonSelect) {
        return m_radioSelect->isChecked();
    } else if (m_selectType == SelectType::CheckBoxSelect) {
        return m_selectBox->isChecked();
    }
    return false;
}

int ComponentsListWidgetItem::getHeight()
{
    return m_realHeight;
}

void ComponentsListWidgetItem::updateSize(int width)
{
    int contentLabelwidth = width - this->layout()->spacing()
            - this->layout()->contentsMargins().left() - this->layout()->contentsMargins().right()
            - m_contentLabel->contentsMargins().left() - m_contentLabel->contentsMargins().right()
            - 26 - 10;

    m_realHeight = m_titleLabel->heightForWidth(width) + m_contentLabel->heightForWidth(width)
            + this->layout()->contentsMargins().bottom() + this->layout()->contentsMargins().top()
            + this->layout()->spacing();

    m_titleLabel->setFixedWidth(contentLabelwidth);
    m_contentLabel->setFixedWidth(contentLabelwidth);

    this->setMaximumSize(width, m_realHeight);
    this->setMinimumSize(width, m_realHeight);
}

void ComponentsListWidgetItem::initUI()
{
    // 标题
    m_titleLabel = new QLabel;
    m_titleLabel->setWordWrap(true);
    m_titleLabel->adjustSize();
    QFont font;
    font.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(font);

    // 子标题
    m_contentLabel = new QLabel;
    m_contentLabel->setWordWrap(true);
    m_contentLabel->adjustSize();

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);
    vlayout->addWidget(m_titleLabel);
    vlayout->addWidget(m_contentLabel);
    QWidget *vwidget = new QWidget;
    vwidget->setLayout(vlayout);

    // 根据调用者传入的 m_selectType 决定创建多选框还是单选框
    QHBoxLayout *hlayout = new QHBoxLayout(this);
    hlayout->setContentsMargins(0, 0, 0, 0);
    if (m_selectType == SelectType::RadioButtonSelect) {
        m_radioSelect = new QRadioButton;
        m_radioSelect->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_radioSelect->setMaximumWidth(26);
        m_radioSelect->setMinimumWidth(26);
        hlayout->addWidget(m_radioSelect);
    } else if (m_selectType == SelectType::CheckBoxSelect) {
        m_selectBox = new QCheckBox;
        m_selectBox->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_selectBox->setMaximumWidth(26);
        m_selectBox->setMinimumWidth(26);
        hlayout->addWidget(m_selectBox);
    }

    hlayout->addWidget(vwidget);

    // 设置背景 widget
    QWidget *mainwidget = new QWidget;
    mainwidget->setObjectName("ComponentsListWidget");
    mainwidget->setStyleSheet("QWidget#ComponentsListWidget{ background:rgba(0,0,0,0.03); border-radius:8px; }\
                              QWidget#ComponentsListWidget::hover { background:rgba(255,255,255,1) rgba(0,0,0,0.1); border-radius:8px; }");
    mainwidget->setLayout(hlayout);

    QHBoxLayout *mainwidgetlayout = new QHBoxLayout(this);
    mainwidgetlayout->setContentsMargins(0, 0, 0, 0);
    mainwidgetlayout->setSpacing(0);
    mainwidgetlayout->addWidget(mainwidget);

    this->setLayout(mainwidgetlayout);
}
