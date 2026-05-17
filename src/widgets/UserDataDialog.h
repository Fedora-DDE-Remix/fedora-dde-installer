#ifndef USERDATADIALOG_H
#define USERDATADIALOG_H

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>
#include "structs.h"

DWIDGET_USE_NAMESPACE

enum class BtnSelectState {
    close = 0,
    cancel,
    confirm
};

class TitleWidget;
class UserDataDialog : public DDialog {
    Q_OBJECT


public:
    explicit UserDataDialog(KeepDataError errCode, QString oldOSVersion, QWidget *parent = Q_NULLPTR);
    BtnSelectState getBtnSelectState(){ return m_selectState; }

private:
    BtnSelectState m_selectState;
};

#endif // USERDATADIALOG_H
