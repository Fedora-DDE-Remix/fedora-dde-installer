#include "ShadowWidget.h"

#include <QPainter>
#include <QFont>
#include <QStackedLayout>
#include <QVBoxLayout>

ShadowWidget::ShadowWidget(QWidget* parent)
    : QWidget(parent)
    , m_childFrame(nullptr)
{
}

void ShadowWidget::setContent(QWidget* inter)
{
    m_childFrame = inter;
}

void ShadowWidget::paintEvent(QPaintEvent *event)
{
    if (m_childFrame != nullptr) {
        int basecolor = 155;
        for (int i = 0; i < 10; i++) {
            QPainter painter;
            painter.begin(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            //每次循环，颜色值加10
            painter.setPen(QPen(QColor(basecolor + i * 10, basecolor + i * 10, basecolor + i * 10, 30), 2));
            //控制阴影的方向
            QRectF rect(20 - i, 20 - i, m_childFrame->width() + (2 * i), m_childFrame->height() + (2 * i));
            painter.drawRoundedRect(rect, 20, 20);
            painter.end();
        }
    }
}
