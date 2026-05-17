#ifndef RESETLISTDIALOG_H
#define RESETLISTDIALOG_H

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class TitleWidget;
class ResetListDialog : public DDialog {
    Q_OBJECT
public:
    explicit ResetListDialog(QWidget *parent = Q_NULLPTR);
};

#endif // RESETLISTDIALOG_H
