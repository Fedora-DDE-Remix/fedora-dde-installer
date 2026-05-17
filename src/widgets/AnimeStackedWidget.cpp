#include "AnimeStackedWidget.h"
#include <QLabel>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

AnimeStackedWidget::AnimeStackedWidget(QWidget *parent):QStackedWidget (parent)
{
    m_label = new QLabel(this);
}

void AnimeStackedWidget::setCurrentIndex(int index)
{
    if(index < 0 || index >= count() || index == currentIndex()) {
        return;
    }

    if(currentIndex() != -1) {
        int oldIndex = currentIndex();
        //m_label->setPixmap(currentWidget()->grab());
        m_label->setGeometry(0, 0, this->width(), this->height());
        m_label->show();

        QStackedWidget::setCurrentIndex(index);

        QParallelAnimationGroup *animeGroup = new QParallelAnimationGroup;
        QPropertyAnimation *animeBack = new QPropertyAnimation;
        animeBack->setDuration(600);
        animeBack->setPropertyName("pos");
        animeBack->setEasingCurve(QEasingCurve::OutCubic);
        animeBack->setTargetObject(m_label);
        QPropertyAnimation *animeForward = new QPropertyAnimation;
        animeForward->setDuration(600);
        animeForward->setPropertyName("pos");
        animeForward->setEasingCurve(QEasingCurve::OutCubic);
        animeForward->setTargetObject(currentWidget());
        animeGroup->addAnimation(animeBack);
        animeGroup->addAnimation(animeForward);

        if(oldIndex < index) {
            animeBack->setStartValue(QPoint(0, 0));
            animeBack->setEndValue(QPoint(-1 * width(), 0));
            animeForward->setStartValue(QPoint(width(), 0));
            animeForward->setEndValue(QPoint(0, 0));
        } else {
            animeBack->setStartValue(QPoint(0, 0));
            animeBack->setEndValue(QPoint(width(), 0));
            animeForward->setStartValue(QPoint(-1 * width(), 0));
            animeForward->setEndValue(QPoint(0, 0));
        }

        connect(animeGroup, &QAbstractAnimation::finished, [=]{
            m_label->hide();
        });

        animeGroup->start(QAbstractAnimation::DeleteWhenStopped);
    }
}
