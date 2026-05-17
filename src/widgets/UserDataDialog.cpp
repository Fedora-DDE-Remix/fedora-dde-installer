#include "UserDataDialog.h"
#include "TitleWidget.h"
#include "globals.h"
#include <DTipLabel>
#include <QHBoxLayout>
#include <QScreen>

UserDataDialog::UserDataDialog(KeepDataError errCode, QString oldOSVersion, QWidget *parent):
    DDialog(parent)
{
    setFixedWidth(440);
    setTitle(::QObject::tr("Can't Keep user data"));
    DTipLabel *tipText = new DTipLabel;
    tipText->setWordWrap(true);
    tipText->setMinimumHeight(60);
    bool useCancelBtn = false;
    switch (errCode) {
    case KeepDataError::BOOT_ERROR:
        tipText->setText(::QObject::tr("The selected disk uses a different boot method than the configuration of this installation and cannot retain your data (/data/home/)"));
        break;
    case KeepDataError::VERSION_MATCH_ERROR:
        tipText->setText(QString(::QObject::tr("The data to be retained is from the %1 system version. Continuing with the installation will not preserve your data. Please confirm if you wish to proceed?")).arg(oldOSVersion));
        useCancelBtn = true;
        break;
    case KeepDataError::OTHER_ERROR:
        tipText->setText(::QObject::tr("You cannot keep the old user data if selecting this disk, as it must have the same partitioning scheme as before."));
        break;
    default:
        break;
    }

    QWidget *content = new QWidget;
    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setContentsMargins(30, 0, 30, 0);
    content->setLayout(contentLayout);
    contentLayout->addWidget(tipText);
    addSpacing(10);
    addContent(content);
    addSpacing(10);
    if (useCancelBtn) {
        addButton(::QObject::tr("Cancel", "button"), true);
    }
    addButton(::QObject::tr("Confirm", "button"), true);


    connect(this, &DDialog::buttonClicked, this, [=](int index, const QString &text) {
        if (index == 0) {
            m_selectState = BtnSelectState::cancel;
        } else if (index == 1) {
            m_selectState = BtnSelectState::confirm;
        } else {
            m_selectState = BtnSelectState::close;
        }
    });

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}
