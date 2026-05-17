#ifndef ANIMATIONWIDGET_H
#define ANIMATIONWIDGET_H

#include <QWidget>
#include <QStringList>

class QPropertyAnimation;
class QLabel;
class QTimer;

class AnimationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AnimationWidget(QWidget *partent = nullptr);
    void setAnimations(const QStringList &animations);
    void backAnimation();
    void nextAnimation();
public slots:
    void startAnimation();
signals:
    void animationChange(const QString &);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void updateAnimation(bool isClockwise = true);
    void updateAnimation(const QSize &size, bool isClockwise = true);
private:
     QPropertyAnimation *m_runAnimation     = nullptr;
     QPropertyAnimation *m_enterAnimation   = nullptr;
     QPropertyAnimation *m_leaveAnimation   = nullptr;
     QPropertyAnimation *m_opacityAnimation = nullptr;
     QLabel *m_animationPageBefore = nullptr;
     QLabel *m_animationPage = nullptr;
     QStringList m_animationList;
     QTimer *m_timer = nullptr;
     int m_animationPos = -1;
     int m_widthOffset = 40;
     int m_animationSpeedBuffer = 150;
};

#endif // ANIMATIONWIDGET_H
