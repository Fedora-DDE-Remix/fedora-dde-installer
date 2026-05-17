#include "WrapWidget.h"

WrapWidget::WrapWidget(QWidget *headWidget, QWidget *contentWidget, QWidget *parent)
    : QWidget (parent)
{
    QVBoxLayout *mainLayout = nullptr;

    mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    if (headWidget != nullptr) {
        mainLayout->addWidget(headWidget, 0, Qt::AlignHCenter | Qt::AlignVCenter);
    }

    if (contentWidget != nullptr) {
        mainLayout->addSpacing(35);
        mainLayout->addWidget(contentWidget, 0, Qt::AlignHCenter | Qt::AlignVCenter);
        contentWidget->setVisible(false);

        connect(this, &WrapWidget::headClicked, contentWidget, [=] (const bool isVisible){
            contentWidget->setVisible(isVisible);
        });
    }

    setLayout(mainLayout);
}
