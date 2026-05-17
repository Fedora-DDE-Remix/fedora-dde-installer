#include "ResetListDialog.h"
#include "TitleWidget.h"
#include "globals.h"

#include <QHBoxLayout>
#include <QScreen>

ResetListDialog::ResetListDialog(QWidget *parent):
    DDialog(parent)
{
    setTitle(::QObject::tr("Reset List"));
    setMessage(::QObject::tr("Are you sure you want to reset the current configuration for partitions?"));
    addButton(::QObject::tr("Cancel", "button"));
    addButton(::QObject::tr("Reset", "button"), true, DDialog::ButtonRecommend);
    // connect(this, &DDialog::visibleChanged, [=](bool visible){
    //     if(visible) {
    //         this->moveToCenter();
    //     }
    // });

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}
