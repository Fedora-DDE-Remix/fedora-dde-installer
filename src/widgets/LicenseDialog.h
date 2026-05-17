#ifndef LICENSEDIALOG_H
#define LICENSEDIALOG_H

#include <DDialog>
#include <DWidget>
#include <DBlurEffectWidget>
#include <DMainWindow>


DWIDGET_USE_NAMESPACE
class LicenseDialog : public DDialog
{
    Q_OBJECT
public:
    explicit LicenseDialog(const QString text, DDialog *parent = nullptr);
    ~LicenseDialog();
};

#endif // LICENSEDIALOG_H
