#ifndef ENCRYPTIONPASSWORDDIALOG_H
#define ENCRYPTIONPASSWORDDIALOG_H

#include <DDialog>
#include <DPasswordEdit>

DWIDGET_USE_NAMESPACE

// A fresh passphrase is required for a new encrypted root. There is no keyfile,
// unattended unlock, or recovery-key export path in this dialog.
class EncryptionPasswordDialog : public DDialog
{
public:
    explicit EncryptionPasswordDialog(QWidget *parent = nullptr);
    ~EncryptionPasswordDialog() override;

private:
    DPasswordEdit *m_passphrase;
    DPasswordEdit *m_confirmation;
};

#endif
