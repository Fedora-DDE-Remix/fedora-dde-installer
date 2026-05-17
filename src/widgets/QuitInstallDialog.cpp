#include "QuitInstallDialog.h"

#include "globals.h"

#include <QHBoxLayout>
#include <DTipLabel>
#include <QScreen>

QuitInstallDialog::QuitInstallDialog(QWidget *parent):
    DDialog(parent)
{
    setFixedWidth(440);
    setTitle(::QObject::tr("Exit"));
    QLabel *messageLabel = new QLabel(this);
    messageLabel->setFixedWidth(400);
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setText(::QObject::tr("Relevant operations you made in the installation process will not take effect."));
    addSpacing(15);
    addContent(messageLabel, Qt::AlignCenter);
    addSpacing(15);
    addButton(::QObject::tr("Cancel", "button"));
    addButton(::QObject::tr("Confirm", "button"), true, DDialog::ButtonWarning);
    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}
