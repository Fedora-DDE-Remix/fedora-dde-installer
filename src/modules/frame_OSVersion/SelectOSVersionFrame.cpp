//
// Created by dtx on 2024/8/22.
//

#include "SelectOSVersionFrame.h"
#include "TitleWidget.h"
#include "EditionItemFrame.h"
#include "CommonFunc.h"
#include "globals.h"

#include <QKeyEvent>

const int ITEM_FRAME_HEIGHT = 240;
const int ITEM_FRAME_WIDTH = 240;
const int ITEM_FRAME_HEIGHT_VERTICAL_LAYOUT = 500;
const int ITEM_FRAME_WIDTH_VERTICAL_LAYOUT = 650;

SelectOSVersionFrame::SelectOSVersionFrame(QWidget *parent) :FramePluginInterface (parent)
{
    initUI();

    initConnection();

    initListContent();

    qApp->installEventFilter(this);
}

SelectOSVersionFrame::~SelectOSVersionFrame()
{
}

bool SelectOSVersionFrame::load()
{
#ifndef QT_DEBUG
    return CommonFunc::getSettingsBool("DI_EDITION_PLUGIN_LOAD");
#else
    return true;
#endif
}

void SelectOSVersionFrame::initUI()
{
    m_title = new TitleWidget;

    m_OSVersionListView = new DListView(this);
    m_model = new QStandardItemModel(m_OSVersionListView);
    m_OSVersionListView->setModel(m_model);
    m_OSVersionListView->setObjectName("SelectOSVersionFrameListView");
    m_OSVersionListView->setFrameShape(QFrame::Shape::NoFrame);
    // 不能编辑触发
    m_OSVersionListView->setEditTriggers(QListView::NoEditTriggers);
    // 不可以拖拽
    m_OSVersionListView->setDragEnabled(false);
    m_OSVersionListView->clearSelection();
    m_OSVersionListView->clearFocus();
    m_OSVersionListView->clearMask();
    m_OSVersionListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_OSVersionListView->setFixedWidth(ITEM_FRAME_WIDTH_VERTICAL_LAYOUT);
    m_OSVersionListView->setFixedHeight(ITEM_FRAME_HEIGHT_VERTICAL_LAYOUT);
    m_OSVersionListView->setResizeMode(QListView::Adjust);
    m_OSVersionListView->setViewMode(QListView::IconMode);
    m_OSVersionListView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_OSVersionListView->setContentsMargins(0, 0, 0, 0);
    m_OSVersionListView->setSpacing(32);
    // 设置其他属性
    m_OSVersionListView->setTextElideMode(Qt::ElideRight);
    m_OSVersionListView->setFocusPolicy(Qt::TabFocus);
    m_OSVersionListView->setStyleSheet("QListView::item:focus{border:2px solid; border-radius:5px; border-color:rgb(1, 128, 255)}");

    // 下一步
    m_nextBnt = new DSuggestButton;
    m_nextBnt->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBnt->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_nextBnt->setText(::QObject::tr("Next"));
    m_nextBnt->setEnabled(false);
    m_nextBnt->setDefault(true);

    // 主布局
    m_contentLayout->addWidget(m_title);
    m_contentLayout->addStretch();
    m_contentLayout->addWidget(m_OSVersionListView, 0, Qt::AlignHCenter);
    m_contentLayout->addStretch();
    m_contentLayout->addWidget(m_nextBnt, 0, Qt::AlignHCenter | Qt::AlignBottom);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());
}

void SelectOSVersionFrame::initConnection()
{
    connect(m_OSVersionListView, &QListView::clicked, this, &SelectOSVersionFrame::onItemClicked);
    connect(m_nextBnt, &QAbstractButton::clicked, this, &SelectOSVersionFrame::next);
}

void SelectOSVersionFrame::updateTs()
{
    m_title->setTitleLabelProperty(::QObject::tr("Installation Version"));
    m_title->setTipsLabelProperty(::QObject::tr("Please select the appropriate system version"));
    m_nextBnt->setText(::QObject::tr("Next"));
    int rowCount = m_model->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        EditionItemFrame *item = qobject_cast<EditionItemFrame *>(m_OSVersionListView->indexWidget(m_model->index(i, 0)));
        if (item->getItemType() == ItemFrameType::ProfessionalType) {
            item->setTitleText(::QObject::tr("Professional Version"));
        } else if (item->getItemType() == ItemFrameType::EducationType) {
            item->setTitleText(::QObject::tr("Education Version"));
        }
    }
}

void SelectOSVersionFrame::initListContent()
{
    // 按钮  中间标题
    QList<QPair<ItemFrameType, QString>> typeIcons = { {ItemFrameType::ProfessionalType, ":/images/icon_nav_professional.svg"},
                                                    {ItemFrameType::EducationType, ":/images/icon_nav_education.svg"} };

    for (int i = 0; i < typeIcons.size(); i++) {
        EditionItemFrame *itemFrame = new EditionItemFrame(m_OSVersionListView);
        QStandardItem *item = new QStandardItem;
        item->setSizeHint(QSize(ITEM_FRAME_WIDTH + 15, ITEM_FRAME_HEIGHT + 10));
        item->setDragEnabled(false);
        itemFrame->setItemType(typeIcons.at(i).first);
        m_model->appendRow(item);
        itemFrame->setFixedSize(ITEM_FRAME_WIDTH, ITEM_FRAME_HEIGHT);
        itemFrame->setIconPixmap(typeIcons.at(i).second, QSize(96, 96));
        m_OSVersionListView->setIndexWidget(m_model->index(i, 0), itemFrame);
    }
}

bool SelectOSVersionFrame::eventFilter(QObject *watched, QEvent *event)
{
    if (watched->objectName() == "SelectOSVersionFrameListView") {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *key = dynamic_cast<QKeyEvent*>(event);
            if (key != nullptr) {
                int key_value = key->key();
                switch (key_value) {
                    case Qt::Key_Return: {
                        if (m_OSVersionListView->currentIndex().isValid()) {
                            Q_EMIT m_OSVersionListView->clicked(m_OSVersionListView->currentIndex());
                        }
                    }
                    default: qInfo() << "key pressed not thing to do";
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void SelectOSVersionFrame::onItemClicked(const QModelIndex &index)
{
    if (m_OSVersionListView->indexWidget(index) == nullptr) {
        return;
    }

    EditionItemFrame *itemFrame = qobject_cast<EditionItemFrame *>(m_OSVersionListView->indexWidget(index));

    if  (itemFrame->getItemType() == ItemFrameType::ProfessionalType) {
        CommonFunc::setSettingsValue("DI_EDITION_NAME", "Professional");
    } else if (itemFrame->getItemType() == ItemFrameType::EducationType) {
        CommonFunc::setSettingsValue("DI_EDITION_NAME", "E");
    }

    for (int i = 0; i < m_model->rowCount(); ++i) {
        EditionItemFrame *item = qobject_cast<EditionItemFrame *>(m_OSVersionListView->indexWidget(m_model->index(i, 0)));
        item->setCheckState(itemFrame == item);
    }

    m_nextBnt->setEnabled(true);
}