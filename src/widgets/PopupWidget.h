#ifndef POPFRAMEWIDGET_H
#define POPFRAMEWIDGET_H

#include <DFrame>

DWIDGET_USE_NAMESPACE

class ShadowWidget;

class PopupWidget : public DFrame
{
    Q_OBJECT

public:
    explicit PopupWidget(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    ShadowWidget *m_shadowWidget = nullptr;
};

#endif // POPFRAMEWIDGET_H
