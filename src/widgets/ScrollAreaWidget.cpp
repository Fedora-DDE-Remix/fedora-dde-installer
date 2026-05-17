#include "ScrollAreaWidget.h"

#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>

ScrollAreaWidget::ScrollAreaWidget(QWidget *showWidget)
{
    QHBoxLayout *toolLayout = new QHBoxLayout;
    toolLayout->setContentsMargins(0, 0, 0, 0);
    toolLayout->addWidget(showWidget);

    QWidget* scrollWidget = new QWidget;
    scrollWidget->setLayout(toolLayout);

    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    this->setWidgetResizable(true);
    this->setFocusPolicy(Qt::TabFocus);
    this->setFrameStyle(QFrame::NoFrame);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setContextMenuPolicy(Qt::NoContextMenu);
    this->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    this->horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    this->setContentsMargins(0, 0, 0, 0);
    this->setWidget(scrollWidget);
    this->setStyleSheet("QScrollBar:vertical{width:8px;background:rgba(0,0,0,0%);margin:0px,0px,0px,0px;padding-top:9px;padding-bottom:9px;}"
                        "QScrollBar::handle:vertical{width:8px;background:rgba(0,0,0,25%);border-radius:4px;min-height:20;}"
                        "QScrollBar::handle:vertical:hover{width:8px;background:rgba(0,0,0,50%);border-radius:4px;min-height:20;}"
                        "QScrollBar::add-line:vertical {border: 0px;width: 0px;subcontrol-position: top;subcontrol-origin: margin;}"
                        "QScrollBar::sub-line:vertical {border: 0px;width: 0px;subcontrol-position: bottom;subcontrol-origin: margin;}");
}
