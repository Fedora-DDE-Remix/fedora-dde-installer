#include "SelectComponentFrame.h"

#include "CommonFunc.h"
#include "ComponentsListWidgetItem.h"
#include "ComponentsManager.h"
#include "TitleWidget.h"
#include "globals.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QCheckBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QLabel>
#include <QDir>

SelectComponentFrame::SelectComponentFrame(QWidget *parent):
    FramePluginInterface(parent)
{
    setObjectName("SelectComponentFrame");
    m_componentsManager = new ComponentsManager();
    m_componentsManager->initData();
    initUI();
    initConnect();
}

bool SelectComponentFrame::load()
{
    return !CommonFunc::isGhost();
}

void SelectComponentFrame::nextButtonClickedSlot()
{
    QListWidgetItem *basecomponentitem = m_baseComponentListWidget->currentItem();
    if (basecomponentitem) {
        QString itemname = (basecomponentitem->data(Qt::UserRole)).value<QString>();
        QStringList selects = (basecomponentitem->data(Qt::UserRole + 1)).value<QStringList>();
        m_componentsManager->saveData(itemname, selects);
    }

    emit next();
}

void SelectComponentFrame::baseComponentListWidgetClickedSlot(QListWidgetItem *item)
{
    // 设置 item 中 widget 的选中状态
    ComponentsListWidgetItem* listitemwidget = dynamic_cast<ComponentsListWidgetItem*>(m_baseComponentListWidget->itemWidget(item));
    listitemwidget->setSelect(true);

    QString itemname = (item->data(Qt::UserRole)).value<QString>();
    setRightList(m_componentsManager->getPages(itemname));

    // 设置全选控件的选中状态
    m_selectAllBox->setChecked(checkAllExtraSelected());
}

void SelectComponentFrame::extraComponentListWidgetClickedSlot(QListWidgetItem *item)
{
    // 设置 item 中 widget 的选中状态
    ComponentsListWidgetItem* listitemwidget = dynamic_cast<ComponentsListWidgetItem*>(m_extraComponentListWidget->itemWidget(item));
    listitemwidget->setSelect(!listitemwidget->isSelected());

    // 重置 m_baseComponentList 的 item 中记录的选中情况
    QString itemname = (item->data(Qt::UserRole)).value<QString>();
    QStringList selects = getSlectList();
    if( listitemwidget->isSelected() ) {
        selects.append(itemname);
    } else {
        selects.removeOne(itemname);
    }
    setSlectList(selects);

    // 设置全选控件的选中状态
    m_selectAllBox->setChecked(checkAllExtraSelected());
}

void SelectComponentFrame::selectAllBoxClickedSlot()
{
    // 重置 m_baseComponentListWidget 的 item 中记录的所有 choice 的选中情况
    QStringList selects = {};
    bool isselectall = m_selectAllBox->isChecked();
    for (int i = 0; i < m_extraComponentListWidget->count(); i++) {
        QListWidgetItem *item = m_extraComponentListWidget->item(i);
        ComponentsListWidgetItem* listitemwidget = dynamic_cast<ComponentsListWidgetItem*>(m_extraComponentListWidget->itemWidget(item));
        listitemwidget->setSelect(isselectall);
        if (isselectall) {
            selects.append(item->data(Qt::UserRole).value<QString>());
        }
    }
    setSlectList(selects);
}

void SelectComponentFrame::initUI()
{
    // 下一步按钮
    m_nextBnt = new DSuggestButton(this);
    m_nextBnt->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBnt->setText(::QObject::tr("Next"));
    m_nextBnt->setDefault(true);


    // 标题栏
    m_titleWidget = new TitleWidget(this);
    QHBoxLayout *titleMainLayout = new QHBoxLayout;
    titleMainLayout->addStretch();
    titleMainLayout->addWidget(m_titleWidget);
    titleMainLayout->addStretch();

    // “基本服务” 和 “附加选项” 的标题控件
    QFont font;
    font.setWeight(QFont::DemiBold);
    m_leftTitleLabel = new QLabel(::QObject::tr("Basic Environment"));
    m_leftTitleLabel->setObjectName("serverTypeLabel");
    m_leftTitleLabel->setFont(font);
    m_rightTitleLabel = new QLabel(::QObject::tr("Add-Ons"));
    m_rightTitleLabel->setObjectName("componentLabel");
    m_rightTitleLabel->setFont(font);
    QWidget *middlewidget = new QWidget(this);
    QHBoxLayout *middlewidgetlayout = new QHBoxLayout(this);
    middlewidgetlayout->setContentsMargins(10, 10, 10, 10);
    middlewidgetlayout->addWidget(m_leftTitleLabel, 0, Qt::AlignHCenter);
    middlewidgetlayout->addWidget(m_rightTitleLabel, 0, Qt::AlignHCenter);
    middlewidget->setLayout(middlewidgetlayout);

    // 左侧列表
    QWidget *leftwidget = new QWidget(this);
    QHBoxLayout *leftlayout = new QHBoxLayout(this);
    leftlayout->setContentsMargins(8, 8, 2, 8);
    leftwidget->setLayout(leftlayout);
    m_baseComponentListWidget = new QListWidget(this);
    m_baseComponentListWidget->setSpacing(6);
    m_baseComponentListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_baseComponentListWidget->setObjectName("baseComponentListWidget");
    m_baseComponentListWidget->setStyleSheet( "QListWidget#baseComponentListWidget { border:none; } \
                                              QListWidget#baseComponentListWidget::item { height:45px; border:none; background: transparent; }" );
    leftlayout->addWidget(m_baseComponentListWidget);

    // 右侧列表
    QWidget *rightwidget = new QWidget(this);
    QVBoxLayout *rightlayout = new QVBoxLayout(this);
    rightlayout->setContentsMargins(8, 8, 2, 8);
    rightwidget->setLayout(rightlayout);
    m_extraComponentListWidget = new QListWidget(this);
    m_extraComponentListWidget->setSpacing(6);
    m_extraComponentListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_extraComponentListWidget->setObjectName("extraComponentListWidget");
    m_extraComponentListWidget->setStyleSheet( "QListWidget#extraComponentListWidget { border:none; } \
                                              QListWidget#extraComponentListWidget::item { height:45px; border:none; background: transparent; }");
    rightlayout->addWidget(m_extraComponentListWidget);

    QVBoxLayout *selectalllayout = new QVBoxLayout(this);
    m_selectAllBox = new QCheckBox(::QObject::tr("Select All"));
    selectalllayout->addWidget(m_selectAllBox);
    QWidget *selectalllwidget = new QWidget(this);
    selectalllwidget->setLayout(selectalllayout);
    rightlayout->addWidget(selectalllwidget);

    // 两个列表中间的分割线
    QWidget *vspacingbar = new QWidget(this);
    vspacingbar->setObjectName("VSpacingBar");
    vspacingbar->setStyleSheet("QWidget#VSpacingBar{ background:rgb(194, 193, 195); }");
    vspacingbar->setFixedWidth(1);

    QHBoxLayout *inercenterlayout = new QHBoxLayout(this);
    inercenterlayout->setContentsMargins(0, 0, 0, 0);
    inercenterlayout->setSpacing(0);
    inercenterlayout->addWidget(leftwidget);
    inercenterlayout->addWidget(vspacingbar);
    inercenterlayout->addWidget(rightwidget);
    QWidget *inercenterWidget = new QWidget(this);
    inercenterWidget->setObjectName("SelectComponentFrameInerCenterWidget");
    inercenterWidget->setStyleSheet("QWidget#SelectComponentFrameInerCenterWidget{ background:rgb(255, 255, 255); \
                                    border-radius:8px; border:1px solid; border-color:rgb(194, 193, 195); padding:2px 4px; }");
    inercenterWidget->setLayout(inercenterlayout);

    QHBoxLayout *centerlayout = new QHBoxLayout(this);
    centerlayout->setContentsMargins(60, 0, 60, 0);
    centerlayout->addWidget(inercenterWidget);

    // 组件列表控件
    QWidget *componentslistwidget = new QWidget;
    componentslistwidget->setLayout(centerlayout);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addSpacing(CommonFunc::getTopMargin());
    main_layout->addLayout(titleMainLayout);
    main_layout->addWidget(middlewidget);
    main_layout->addWidget(componentslistwidget);
    main_layout->addSpacing(20);
    main_layout->addWidget(m_nextBnt, 0, Qt::AlignCenter);
    main_layout->addSpacing(CommonFunc::isBigScreen() ? 10 : 8);

    this->setLayout(main_layout);

    // 左侧列表管理的RadioButton单选的控件
    m_buttonGroup = new QButtonGroup(this);
}

void SelectComponentFrame::initConnect()
{
    connect(m_nextBnt, &QPushButton::clicked, this, &SelectComponentFrame::nextButtonClickedSlot);
    connect(m_baseComponentListWidget, &QListWidget::itemClicked, this, &SelectComponentFrame::baseComponentListWidgetClickedSlot);
    connect(m_extraComponentListWidget, &QListWidget::itemClicked, this, &SelectComponentFrame::extraComponentListWidgetClickedSlot);
    connect(m_selectAllBox, &QCheckBox::clicked, this, &SelectComponentFrame::selectAllBoxClickedSlot);
}

void SelectComponentFrame::updateTs()
{
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Select Components"), "");
    m_titleWidget->setTipsLabelProperty(::QObject::tr("Select Components"), "font-size:16px;font-weight:Medium;");
    m_leftTitleLabel->setText(::QObject::tr("Basic Environment"));
    m_rightTitleLabel->setText(::QObject::tr("Add-Ons"));
    m_selectAllBox->setText(::QObject::tr("Select All"));
    m_nextBnt->setText(::QObject::tr("Next"));
    if (m_baseComponentListWidget->count() != 0) {
        setLeftList(m_componentsManager->getNames());
    }
}

void SelectComponentFrame::showEvent(QShowEvent *event)
{
    if (m_baseComponentListWidget->count() == 0) {
        setLeftList(m_componentsManager->getNames());
    }

    FramePluginInterface::showEvent(event);
}

void SelectComponentFrame::setLeftList(const QStringList &listinfo)
{
    if(listinfo.size() == 0) {
        return;
    }
    m_baseComponentListWidget->clear();
    QList<QAbstractButton*> testbuttons = m_buttonGroup->buttons();
    for (int i = 0; i < testbuttons.size(); i++) {
        m_buttonGroup->removeButton(testbuttons.at(i));
    }

    for (int i = 0; i < listinfo.size(); i++) {
        ComponentsListWidgetItem* listwidgetitem = new ComponentsListWidgetItem(nullptr, ComponentsListWidgetItem::SelectType::RadioButtonSelect);
        QPair<QString, QString> componentnames = m_componentsManager->getTSText(listinfo.at(i));
        listwidgetitem->setShowText(componentnames.first, componentnames.second);
        listwidgetitem->updateSize(m_baseComponentListWidget->width() - m_baseComponentListWidget->verticalScrollBar()->sizeHint().width()); // 减掉滚动条的宽度
        m_buttonGroup->addButton(listwidgetitem->radioButton(), i);

        QListWidgetItem* listitem = new QListWidgetItem(m_baseComponentListWidget);
        listitem->setSizeHint(QSize(m_baseComponentListWidget->width(), listwidgetitem->getHeight()));
        m_baseComponentListWidget->addItem(listitem);
        m_baseComponentListWidget->setItemWidget(listitem, listwidgetitem);
        // 把组件名记录到item数据中
        listitem->setData(Qt::UserRole, listinfo.at(i));
        listitem->setData(Qt::UserRole + 1, {});
    }

    m_baseComponentListWidget->setCurrentRow(0);
    QListWidgetItem* item = m_baseComponentListWidget->item(0);
    baseComponentListWidgetClickedSlot(item);
}

void SelectComponentFrame::setRightList(const QStringList &listinfo)
{
    if(listinfo.size() == 0) {
        return;
    }

    QStringList selects = getSlectList();

    m_extraComponentListWidget->clear();
    for (int i = 0; i < listinfo.size(); i++) {
        // 创建具有checkbox组件的itemwidget
        ComponentsListWidgetItem* extralistwidgetitem = new ComponentsListWidgetItem(nullptr, ComponentsListWidgetItem::SelectType::CheckBoxSelect);
        QPair<QString, QString> choicenames = m_componentsManager->getTSText(listinfo.at(i));
        extralistwidgetitem->setShowText(choicenames.first, choicenames.second);
        extralistwidgetitem->updateSize(m_extraComponentListWidget->width() - m_extraComponentListWidget->verticalScrollBar()->sizeHint().width());
        extralistwidgetitem->setSelect(selects.contains(listinfo.at(i)));

        QListWidgetItem* listitem = new QListWidgetItem(m_extraComponentListWidget);
        listitem->setSizeHint(QSize(m_extraComponentListWidget->width(), extralistwidgetitem->getHeight()));
        m_extraComponentListWidget->addItem(listitem);
        m_extraComponentListWidget->setItemWidget(listitem, extralistwidgetitem);
        // 把组件名记录到item数据中
        listitem->setData(Qt::UserRole, listinfo.at(i));
    }

    m_selectAllBox->setChecked(checkAllExtraSelected());
}

bool SelectComponentFrame::checkAllExtraSelected()
{
    // 从 m_baseComponentListWidget 当前选中的 item 的 data 中解析右侧列表项的全选情况
    bool isallselect = true;
    QStringList selects = getSlectList();

    int listcount = m_extraComponentListWidget->count();
    for (int i = 0; i < listcount; i++) {
        QString itemname = (m_extraComponentListWidget->item(i)->data(Qt::UserRole)).value<QString>();
        if (!selects.contains(itemname)) {
            isallselect = false;
            break;
        }
    }
    return isallselect;
}

QStringList SelectComponentFrame::getSlectList()
{
    QStringList selects = {};
    QListWidgetItem* basecomponentitem = m_baseComponentListWidget->currentItem();
    if (basecomponentitem != nullptr) {
        selects = (basecomponentitem->data(Qt::UserRole + 1)).value<QStringList>();
    }
    return selects;
}

void SelectComponentFrame::setSlectList(const QStringList &selectlist)
{
    QListWidgetItem* basecomponentitem = m_baseComponentListWidget->currentItem();
    if (basecomponentitem != nullptr) {
        basecomponentitem->setData(Qt::UserRole + 1, selectlist);
    }
}
