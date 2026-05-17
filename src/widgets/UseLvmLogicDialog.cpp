#include "UseLvmLogicDialog.h"
#include "TitleWidget.h"
#include "globals.h"

#include <QHBoxLayout>

UseLvmLogicDialog::UseLvmLogicDialog(QWidget *parent):
    DDialog(parent)
{
    setOnButtonClickedClose(false);
    setTitle(::QObject::tr("Enable Volume Extension"));
    setMessage(::QObject::tr("Enabling volume extension allows you to expand disk space, but may affect disk performance."));
    addButton(::QObject::tr("Confirm", "button"), true, DDialog::ButtonRecommend);
    connect(getButton(0), &QAbstractButton::clicked, this, &QDialog::accept);
}
