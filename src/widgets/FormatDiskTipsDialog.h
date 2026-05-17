#ifndef FORMATDISKTIPSDIALOG_H
#define FORMATDISKTIPSDIALOG_H

#include "structs.h"

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class FormatDiskTipsDialog : public DDialog
{
    Q_OBJECT
public:
    explicit FormatDiskTipsDialog(const SDeviceInfo &dev, QWidget *parent = Q_NULLPTR);
private:
    DLabel *m_messageLabel = Q_NULLPTR;
    DLabel *m_contentLabel = Q_NULLPTR;
};

#endif // FORMATDISKTIPSDIALOG_H
