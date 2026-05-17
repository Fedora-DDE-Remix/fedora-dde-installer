#ifndef USELVMLOGICDIALOG_H
#define USELVMLOGICDIALOG_H

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class TitleWidget;
class UseLvmLogicDialog : public DDialog {
    Q_OBJECT
public:
    explicit UseLvmLogicDialog(QWidget *parent = Q_NULLPTR);
};

#endif // USELVMLOGICDIALOG_H
