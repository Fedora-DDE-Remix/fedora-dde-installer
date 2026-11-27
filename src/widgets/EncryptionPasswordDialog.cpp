#include "EncryptionPasswordDialog.h"
#include "EncryptionSecret.h"

#include <DLabel>
#include <QAbstractButton>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

EncryptionPasswordDialog::EncryptionPasswordDialog(QWidget *parent)
    : DDialog(parent), m_passphrase(new DPasswordEdit(this)), m_confirmation(new DPasswordEdit(this))
{
    EncryptionSecret::clear();
    setTitle(QObject::tr("Encrypt the installed system"));
    setOnButtonClickedClose(false);
    setFixedWidth(500);
    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    auto *explanation = new DLabel(QObject::tr("System and personal files will be encrypted. The EFI System Partition and /boot remain unencrypted so the computer can start. Enter this passphrase each time you start the installed system."), content);
    explanation->setWordWrap(true);
    layout->addWidget(explanation);
    auto *form = new QFormLayout;
    form->addRow(QObject::tr("Encryption passphrase"), m_passphrase);
    form->addRow(QObject::tr("Confirm passphrase"), m_confirmation);
    layout->addLayout(form);
    auto *hint = new DLabel(QObject::tr("Keep the passphrase safe. It is separate from your login password; losing it means losing access to the encrypted files."), content);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto *error = new DLabel(content);
    error->setWordWrap(true);
    layout->addWidget(error);
    addContent(content);
    const int cancel = addButton(QObject::tr("Cancel"));
    const int confirm = addButton(QObject::tr("Confirm"), true, DDialog::ButtonRecommend);
    getButton(confirm)->setEnabled(false);
    for (auto *edit : {m_passphrase, m_confirmation}) {
        edit->lineEdit()->setMaxLength(EncryptionSecret::MaximumBytes);
        edit->lineEdit()->setInputMethodHints(Qt::ImhHiddenText | Qt::ImhSensitiveData | Qt::ImhNoPredictiveText);
        connect(edit, &DLineEdit::textChanged, this, [=] {
            error->clear();
            getButton(confirm)->setEnabled(!m_passphrase->text().isEmpty() && !m_confirmation->text().isEmpty());
        });
    }
    connect(getButton(cancel), &QAbstractButton::clicked, this, &DDialog::reject);
    connect(getButton(confirm), &QAbstractButton::clicked, this, [=] {
        if (m_passphrase->text() != m_confirmation->text()) {
            error->setText(QObject::tr("The passphrases do not match."));
            return;
        }
        QByteArray bytes = m_passphrase->text().toUtf8();
        QString message;
        if (!EncryptionSecret::store(bytes, message)) {
            error->setText(message);
            return;
        }
        m_passphrase->clear();
        m_confirmation->clear();
        accept();
    });
    m_passphrase->setFocus();
}

EncryptionPasswordDialog::~EncryptionPasswordDialog()
{
    m_passphrase->clear();
    m_confirmation->clear();
}
