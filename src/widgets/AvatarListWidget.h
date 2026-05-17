#ifndef AVATARLISTWIDGET_H
#define AVATARLISTWIDGET_H

#include "PopupWidget.h"

#include <QGridLayout>
#include <DIconButton>
#include <DDialog>
#include <DPushButton>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class AvatarListWidget : public DDialog
{
    Q_OBJECT

public:
    explicit AvatarListWidget(QWidget* parent = nullptr);
    void setAvatars(const QStringList &avatars);
    void setCurrentAvatar(const QString& path);

signals:
    void selectAvatar(const QString& avatarPath);

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent *event) override;
private:
    void onButtonGroupClicked(int id);
    void updateTs();

    QGridLayout* m_gridLayout = nullptr;
    QAbstractButton *m_okBnt = nullptr;
    QAbstractButton *m_cancelBnt = nullptr;

    QButtonGroup *m_btnGroup = nullptr;
};

#endif // AVATARLISTWIDGET_H
