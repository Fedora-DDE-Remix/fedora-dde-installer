//
// Created by uos on 2022/3/18.
//

#include "RegionFormatItemFrame.h"
#include "CommonFunc.h"
#include "SearchableListViewDialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <DFontSizeManager>
#include <QDebug>

RegionFormatItemFrame::RegionFormatItemFrame(QWidget *parent)
    : DFrame(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setObjectName("RegionFormatItemFrame");
    setBackgroundRole(Dtk::Gui::DPalette::Window);
    setLineWidth(0);
    QPalette palette = this->palette();
    // 设置背景颜色为透明
    palette.setColor(QPalette::Window, QColor(0, 0, 0, 0));
    setAutoFillBackground(true);
    setPalette(palette);
    initUI();
}

void RegionFormatItemFrame::setIconPixmap(const QString &iconRes, const QSize &iconSize)
{
    m_icon->setPixmap(CommonFunc::hidpiPixmap(iconRes, iconSize));
}

void RegionFormatItemFrame::setItemType(RegionFormatType type)
{
    m_type = type;
}

RegionFormatType RegionFormatItemFrame::getItemType()
{
    return m_type;
}

void RegionFormatItemFrame::setCheckState(bool checked)
{
//    m_bChecked = checked;
//    if (m_bChecked) {
//        m_checkIcon->setVisible(true);
//        m_checkIcon->setPixmap(CommonFunc::hidpiPixmap(":/images/radio_button.svg", QSize(20, 20)));
//        m_bChecked = true;
//    } else {
//        m_checkIcon->setPixmap(QPixmap());
//        m_bChecked = false;
//    }
}

void RegionFormatItemFrame::setTitleText(const QString &text)
{
    m_title->setText(text);
    m_title->adjustSize();
}

void RegionFormatItemFrame::setDescriptionText(const QString &text)
{
    m_description->setText(text);
    m_description->adjustSize();
}

void RegionFormatItemFrame::initListData(const QStringList &combContents, const QStringList &combEnglishContents, const QString &defaultText)
{
    m_searchableListViewDialog->setListData(combContents, combEnglishContents, defaultText);

    QFontMetrics labelFont(m_comboBoxText->font());
    QString elideNote = labelFont.elidedText(defaultText, Qt::ElideMiddle, m_comboBoxText->width(), Qt::AlignLeft | Qt::AlignBottom | Qt::TextSingleLine);
    m_comboBoxText->setText(elideNote);
    (elideNote.indexOf("…") != -1) ? m_comboBoxText->setToolTip(defaultText) : m_comboBoxText->setToolTip("");
}

int RegionFormatItemFrame::getListDataIndex()
{
    return m_searchableListViewDialog->getCurrentListDataIndex();
}

QString RegionFormatItemFrame::getListDataText()
{
    return m_comboBoxText->toolTip().isEmpty() ? m_comboBoxText->text() : m_comboBoxText->toolTip();
}

void RegionFormatItemFrame::onComboBoxMenuSelected(int index)
{
    // m_comboBoxText->setText(m_comboBox->comboBox()->currentText());
    // Q_EMIT comboxChanged(m_comboBox->comboBox()->currentIndex());
}

void RegionFormatItemFrame::setSearchableListViewShowPos(QPoint pos)
{
    m_SearchableListViewShowPos = pos;
}

void RegionFormatItemFrame::showSearchableListViewDialog()
{
    m_searchableListViewDialog->move(m_button->mapToGlobal(m_SearchableListViewShowPos));
    m_searchableListViewDialog->setVisible(true);
    m_searchableListViewDialog->show();
}

void RegionFormatItemFrame::hideSearchableListViewDialog()
{
    m_searchableListViewDialog->close();
    m_searchableListViewDialog->setVisible(false);
    m_searchableListViewDialog->hide();
}

bool RegionFormatItemFrame::hasShowSearchableListViewDialog()
{
    return m_searchableListViewDialog->isVisible();
}

void RegionFormatItemFrame::initUI()
{
    QHBoxLayout *centralLayout = new QHBoxLayout(this);
    centralLayout->setContentsMargins(0, 12, 0, 12);
    centralLayout->setSpacing(0);
    centralLayout->addSpacing(15);

    // 图标设置
    m_icon = new DLabel;
    m_icon->setAlignment(Qt::AlignCenter);
    centralLayout->addWidget(m_icon, 0, Qt::AlignLeft);
    centralLayout->addSpacing(10);

    // 标题副标题
    QVBoxLayout *titleLayout = new QVBoxLayout();
    m_title = new DLabel;
    m_title->setAlignment(Qt::AlignLeft);
    m_title->setWordWrap(true);
    //m_title->setFixedHeight(60);
    QFont font = m_title->font();
    font.setWeight(QFont::Medium);
    font.setPixelSize(16);
    m_title->setFont(font);

    m_description = new DLabel;
    m_description->setAlignment(Qt::AlignLeft);
    m_description->setWordWrap(true);
    QFont descriptionFont = m_description->font();
    descriptionFont.setPixelSize(12);
    m_description->setFont(descriptionFont);
    m_description->setFixedWidth(400);

    titleLayout->addStretch();
    titleLayout->addWidget(m_title, 0, Qt::AlignLeft);
    titleLayout->addSpacing(3);
    titleLayout->addWidget(m_description, 0, Qt::AlignLeft);
    titleLayout->addStretch();

    centralLayout->addLayout(titleLayout);
    centralLayout->addStretch();

    m_comboBoxText = new DLabel;
    m_comboBoxText->setObjectName("comboBoxText");
    m_comboBoxText->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_comboBoxText->setFixedHeight(30);
    m_comboBoxText->setFixedWidth(150);
    QFont comboBoxTextFont = m_comboBoxText->font();
    comboBoxTextFont.setPixelSize(12);
    m_comboBoxText->setFont(comboBoxTextFont);
    centralLayout->addWidget(m_comboBoxText, 0, Qt::AlignRight);
    centralLayout->addSpacing(10);

    // 
    m_button = new DIconButton;
    m_button->setStyleSheet("background:rgba(0,0,0,0);border:none;");
    m_button->setIconSize(QSize(16, 16));
    m_button->setIcon(QIcon(":/images/combobox_arrow.svg"));
    m_button->setFocusPolicy(Qt::NoFocus);
    connect(m_button, &DIconButton::clicked, this, [this]() {
        emit popSearchableListViewDialog(static_cast<int>(m_type));
    });

    centralLayout->addWidget(m_button, 0, Qt::AlignRight);

    m_searchableListViewDialog = new SearchableListViewDialog(parentWidget());
    m_searchableListViewDialog->setVisible(false);
    connect(m_searchableListViewDialog, &SearchableListViewDialog::textChanged, this, &RegionFormatItemFrame::onCurrentListItemChanged);

    this->setLayout(centralLayout);
}

bool RegionFormatItemFrame::event(QEvent *e)
{
    if (e->type() == QEvent::HoverEnter) {
        setBackgroundRole(DPalette::ObviousBackground);
    } else if (e->type() == QEvent::HoverLeave) {
        setBackgroundRole(DPalette::ItemBackground);
    }

    return QFrame::event(e);
}

void RegionFormatItemFrame::onCurrentListItemChanged(const QString &text)
{
    QFontMetrics labelFont(m_comboBoxText->font());
    QString elideNote = labelFont.elidedText(text, Qt::ElideMiddle, m_comboBoxText->width(), Qt::AlignLeft | Qt::AlignBottom | Qt::TextSingleLine);
    m_comboBoxText->setText(elideNote);
    (elideNote.indexOf("…") != -1) ? m_comboBoxText->setToolTip(text) : m_comboBoxText->setToolTip("");
    Q_EMIT comboxChanged(m_searchableListViewDialog->getCurrentListDataIndex());
}