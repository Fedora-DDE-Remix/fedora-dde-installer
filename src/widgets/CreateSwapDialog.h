#ifndef CREATESWAPDIALOG_H
#define CREATESWAPDIALOG_H

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class TitleWidget;
class CreateSwapDialog : public DDialog {
    Q_OBJECT
public:
    explicit CreateSwapDialog(QWidget *parent = Q_NULLPTR);

private:
    TitleWidget *m_titleWidget = Q_NULLPTR;
    DLabel *m_contentLabel = Q_NULLPTR;
    DPushButton *m_cancelBtn = Q_NULLPTR;
    DSuggestButton *m_okBtn = Q_NULLPTR;
};

#endif // CREATESWAPDIALOG_H
