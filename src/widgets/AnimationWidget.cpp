#include "AnimationWidget.h"
#include "CommonFunc.h"

#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QDebug>

AnimationWidget::AnimationWidget(QWidget *partent):
    QWidget (partent)
{
    // 动画显示透明度设置
    QGraphicsOpacityEffect *pOpacity = new QGraphicsOpacityEffect(this);
    pOpacity->setOpacity(1);

    // 动画图片显示的控件
    m_animationPageBefore = new QLabel(this);
    m_animationPageBefore->setScaledContents(true);
    m_animationPageBefore->setGraphicsEffect(pOpacity);
    m_animationPageBefore->setStyleSheet("QLabel{border-radius:8px}");

    m_animationPage = new QLabel(this);
    m_animationPage->setScaledContents(true);
    m_animationPage->setGraphicsEffect(pOpacity);
    m_animationPage->setStyleSheet("QLabel{border-radius:8px}");

    // 主布局
    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setContentsMargins(0, 0, 0, 0);
    //main_layout->addWidget(m_animationPage);
    this->setLayout(main_layout);

    // 入场动画
    m_enterAnimation = new QPropertyAnimation(m_animationPage, "pos");
    m_enterAnimation->setDuration(500);
    m_enterAnimation->setEasingCurve(QEasingCurve::Linear);

    // 出场动画显示
    m_leaveAnimation = new QPropertyAnimation(m_animationPageBefore, "pos");
    m_leaveAnimation->setDuration(500);
    m_leaveAnimation->setEasingCurve(QEasingCurve::Linear);

    // 渐隐、渐显效果
    m_opacityAnimation = new QPropertyAnimation(pOpacity, "opacity");
    m_opacityAnimation->setDuration(1000);
    m_opacityAnimation->setKeyValueAt(0.0, 0.0);
    m_opacityAnimation->setKeyValueAt(1.0, 1.0);
    m_opacityAnimation->setEasingCurve(QEasingCurve::Linear);

    // 根据大小屏设置动画移动的偏移
    CommonFunc::isBigScreen() ? m_widthOffset = 0 : m_widthOffset = 40;

    m_timer = new QTimer(this);
    m_timer->setInterval(8000);
    connect(m_timer, &QTimer::timeout, this, &AnimationWidget::startAnimation);
}

void AnimationWidget::setAnimations(const QStringList &animations)
{
    m_animationList = animations;
    std::sort(m_animationList.begin(), m_animationList.end(), [](const QString &lhs, const QString &rhs) {
        QString right_lhs = lhs.right(lhs.length() - lhs.lastIndexOf('/') - 1);
        QString right_rhs = rhs.right(rhs.length() - rhs.lastIndexOf('/') - 1);
        int num_lhs = right_lhs.left(2).toInt();
        int num_rhs = right_rhs.left(2).toInt();
        return num_lhs < num_rhs;
    });
}

void AnimationWidget::backAnimation()
{
    this->updateAnimation(false);
}

void AnimationWidget::nextAnimation()
{
    this->updateAnimation();
}

void AnimationWidget::startAnimation()
{
    if (m_animationList.size() > 0) {
        if (m_runAnimation == nullptr) {
            m_animationPos = m_animationList.size() - 1;
            m_runAnimation = m_leaveAnimation;
        }
    } else {
        return;
    }

    this->updateAnimation(true);

    QPropertyAnimation *prevAnimation = m_runAnimation;
    QPropertyAnimation *nextAnimation = prevAnimation == m_enterAnimation ? m_leaveAnimation : m_enterAnimation;

    // 更新动画的起止位置
    m_enterAnimation->setKeyValueAt(1.0, QPoint(m_widthOffset, 0));
    // 设置一个缓动距离
    m_enterAnimation->setKeyValueAt(0.3, QPoint(this->width() - m_animationSpeedBuffer, 0));
    m_enterAnimation->setKeyValueAt(0.0, QPoint((this->width() + m_widthOffset), 0));

    m_leaveAnimation->setKeyValueAt(1.0, QPoint(-(this->width() + m_widthOffset), 0));
    // 设置一个缓动距离
    m_leaveAnimation->setKeyValueAt(0.3, QPoint(-m_animationSpeedBuffer, 0));
    m_leaveAnimation->setKeyValueAt(0.0, QPoint(m_widthOffset, 0));

    prevAnimation->start();
    nextAnimation->start();
    m_opacityAnimation->start();

    m_runAnimation = nextAnimation;

}

void AnimationWidget::showEvent(QShowEvent *event)
{
    m_timer->start();
    return QWidget::showEvent(event);
}

void AnimationWidget::hideEvent(QHideEvent *event)
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }

    return QWidget::hideEvent(event);
}

void AnimationWidget::updateAnimation(bool isClockwise)
{
    this->updateAnimation(CommonFunc::getAnimationPicSize(), isClockwise);
}

void AnimationWidget::updateAnimation(const QSize &size, bool isClockwise)
{
    if (!m_animationList.isEmpty()) {
        int animationBeforePos = m_animationPos;
        if (isClockwise) {
            m_animationPos++;
            m_animationPos = m_animationPos >= m_animationList.size() ? 0 : m_animationPos;
        } else {
            m_animationPos--;
            m_animationPos = m_animationPos < 0 ? m_animationList.size() - 1 : m_animationPos;
        }

        QString pictureNameBefore = m_animationList.at(animationBeforePos);
        QString pictureName = m_animationList.at(m_animationPos);
        Q_EMIT this->animationChange(pictureName);

        m_animationPageBefore->setPixmap(CommonFunc::getRoundRectPixmap(pictureNameBefore, size, 8));
        m_animationPage->setPixmap(CommonFunc::getRoundRectPixmap(pictureName, size, 8));

        QSize picLabelSize = QSize(this->size().width() - (m_widthOffset * 2), this->size().height());
        m_animationPageBefore->setFixedSize(picLabelSize);
        m_animationPage->setFixedSize(picLabelSize);
    }
}
