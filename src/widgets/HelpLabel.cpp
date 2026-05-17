#include "HelpLabel.h"
#include "CommonFunc.h"
#include <QEvent>
#include <QHoverEvent>
#include <QDebug>
#include <QStyleOption>

HelpLabel::HelpLabel(QWidget *parent) : QLabel(parent)
{
    setAttribute(Qt::WA_Hover);
    setPixmap(QPixmap(":/images/help_32px.svg"));
}

bool HelpLabel::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
    {
        QHoverEvent *hoverEv = dynamic_cast<QHoverEvent*>(e);
        if(hoverEv) {
            Q_EMIT hoverEnter();
        }
    }
        break;
    case QEvent::HoverLeave:
        Q_EMIT hoverLeave();
        break;
    default:
        break;
    }

    return QLabel::event(e);
}

void HelpLabel::paintEvent(QPaintEvent *event)
{
    QStyleOption option;
    option.initFrom(this);
    bool bEnabled = (option.state & QStyle::State_Enabled) && (option.state & QStyle::State_Active);
    QIcon::Mode mode = bEnabled ? QIcon::Normal : QIcon::Disabled;
    QPixmap content = QIcon(":/images/help_32px.svg").pixmap(this->size(), mode);
    setPixmap(content);

    QLabel::paintEvent(event);
}
