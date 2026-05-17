//
// Created by uos on 2022/3/18.
//

#ifndef EDITION_ITEMFRAME_H
#define EDITION_ITEMFRAME_H

#include <DFrame>
#include <DLabel>
#include <DPushButton>
#include <QStandardItem>
#include <QPixmap>

DTK_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

enum class ItemFrameType {
    DefaultType = 0,
    ProfessionalType,
    EducationType
};

class EditionItemFrame : public DFrame
{
    Q_OBJECT
public:
    explicit EditionItemFrame(QWidget *parent = nullptr);
    void initUI();
    void setIconPixmap(const QString &iconRes, const QSize &iconSize);
    void setItemType(ItemFrameType type);
    ItemFrameType getItemType();
    void setCheckState(bool checked);
    void setTitleText(const QString &text);
    void setDescriptionText(const QString &text);


Q_SIGNALS:
    void clicked(bool checked);

protected:
    bool event(QEvent *e) override;

private:
    DLabel *m_title = nullptr;
    DLabel *m_description = nullptr;
    DLabel *m_icon = nullptr;
    DLabel *m_checkIcon = nullptr;
    bool m_bChecked = false;
    ItemFrameType m_type = ItemFrameType::DefaultType;
};

#endif //EDITION_ITEMFRAME_H
