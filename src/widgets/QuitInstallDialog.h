#ifndef QUITINSTALLDIALOG_H
#define QUITINSTALLDIALOG_H

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE
class QuitInstallDialog : public DDialog
{
    Q_OBJECT
public:
    explicit QuitInstallDialog(QWidget *parent = Q_NULLPTR);
};

#endif // QUITINSTALLDIALOG_H
