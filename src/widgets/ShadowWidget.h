#ifndef SHADOW_WIDGET_H
#define SHADOW_WIDGET_H

#include <QWidget>

class QVBoxLayout;

class ShadowWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ShadowWidget(QWidget* parent = nullptr);
    void setContent(QWidget* inter);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QWidget* m_childFrame;
};
#endif // PARTITION_SHADOW_WIDGET_H
