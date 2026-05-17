#include "PopupWidget.h"
#include "ShadowWidget.h"

#include <QApplication>
#include <QScreen>

PopupWidget::PopupWidget(QWidget *parent)
    : DFrame(parent)
{

}

void PopupWidget::showEvent(QShowEvent *event)
{
    // 处理阴影效果
    if (this->parentWidget()) {
        if (m_shadowWidget == nullptr) {
            m_shadowWidget = new ShadowWidget(this->parentWidget());

            m_shadowWidget->setAttribute(Qt::WA_ShowModal, true);
            m_shadowWidget->setContent(this);
            m_shadowWidget->setFixedSize(this->width() + 40, this->height() + 40);
        }

        m_shadowWidget->move((m_shadowWidget->parentWidget()->width() - m_shadowWidget->width()) / 2,
                             (m_shadowWidget->parentWidget()->height() - m_shadowWidget->height()) / 2);
        m_shadowWidget->show();

        this->move((this->parentWidget()->width() - this->width()) / 2,
                   (this->parentWidget()->height() - this->height()) / 2);
        this->raise();
    }

    return DFrame::showEvent(event);
}

void PopupWidget::hideEvent(QHideEvent *event)
{
    if (m_shadowWidget != nullptr) {
        m_shadowWidget->hide();
    }

    return DFrame::hideEvent(event);
}
