#include "PartitionDecryptDialog.h"
#include <QFormLayout>
#include <QTimer>
#include <QScreen>
#include "HelpLabel.h"
#include "PasswordManager.h"
#include "CommonFunc.h"

PartitionDecryptDialog::PartitionDecryptDialog(const QList<SPartitionInfo> &partInfo, QWidget *parent):DDialog (parent), m_decrypList(partInfo)
{
    setOnButtonClickedClose(false);
    setFixedWidth(440);
    setTitle(tr("Decrypt disk"));

    QHBoxLayout *messageLayout = new QHBoxLayout;
    m_messageLabel = new DLabel;
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setFixedWidth(360);
    m_messageLabel->setWordWrap(true);
    m_message = tr("Detected that the disk is encrypted, to keep user data while installation, the disk needs to be decrypted first, please enter the disk (%1) encryption passwords");
    m_messageLabel->setText(m_message);
    messageLayout->addStretch();
    messageLayout->addWidget(m_messageLabel, 0, Qt::AlignCenter);
    messageLayout->addStretch();
    QWidget *messageWidget = new QWidget;
    messageWidget->setLayout(messageLayout);

    QWidget *content = new QWidget;
    QHBoxLayout *layout1 = new QHBoxLayout;
    layout1->setContentsMargins(0, 0, 0, 0);
    layout1->setSpacing(0);
    content->setLayout(layout1);

    DLabel *labelPwd = new DLabel(::QObject::tr("Password:"));
    labelPwd->setAlignment(Qt::AlignCenter);
    m_pwdEdit = new DPasswordEdit;
    m_pwdEdit->setPlaceholderText(::QObject::tr("Please input password"));
    layout1->addWidget(labelPwd, 1);
    layout1->addWidget(m_pwdEdit, 5);

    addContent(messageWidget);
    addSpacing(30);
    addContent(content);
    addSpacing(30);
    addButton(::QObject::tr("Cancel"));
    addButton(::QObject::tr("Confirm"), true, DDialog::ButtonRecommend);
    QAbstractButton *btnOk = getButton(1);
    btnOk->setEnabled(false);
    connect(getButton(0), &QAbstractButton::clicked, this, &DDialog::reject);
    connect(getButton(1), &QAbstractButton::clicked, this, &PartitionDecryptDialog::onHandleDecrypt);
    connect(this, &DDialog::visibleChanged, [=](bool visible){
        if(visible) {
            m_pwdEdit->setFocus();
        }
    });
    connect(m_pwdEdit, &DPasswordEdit::textChanged, this, [=](const QString &text){
        btnOk->setEnabled(!text.isEmpty());
    });

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

void PartitionDecryptDialog::showEvent(QShowEvent *event)
{
    updateMessageLabel();
    DDialog::showEvent(event);
}

void PartitionDecryptDialog::updateMessageLabel()
{
    if(!m_decrypList.isEmpty()) {
        QString path = m_decrypList.first().label.isEmpty() ? m_decrypList.first().path : m_decrypList.first().label;
        m_messageLabel->setText(m_message.arg(path));
    }
}

void PartitionDecryptDialog::onHandleDecrypt()
{
    if(m_decrypList.isEmpty()) {
        accept();
        return;
    }

    QString err;
    QString pwd = m_pwdEdit->text().trimmed();
    QString decryptpath = m_decrypList.at(0).path;
    bool bSuccess = CommonFunc::deCryptPartition(pwd.toLocal8Bit().toBase64(), decryptpath, err);
    qInfo() << "decrypt disk:" << bSuccess << err;
    if(bSuccess) {
        m_decrypList.takeFirst();
        if(m_decrypList.isEmpty()) {
            accept();
        } else {
            updateMessageLabel();
            m_pwdEdit->clear();
        }
    } else {
        m_pwdEdit->showAlertMessage(tr("Failed to decrypt the disk"));
    }
}
