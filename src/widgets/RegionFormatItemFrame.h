//
// Created by uos on 2022/3/18.
//

#ifndef REGIONFORMAT_ITEMFRAME_H
#define REGIONFORMAT_ITEMFRAME_H

#include "IconComboBox.h"

#include <DFrame>
#include <DLabel>
#include <DPushButton>
#include <QStandardItem>
#include <QPixmap>
//#include <QComboBox>

DTK_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

class SearchableListViewDialog;

enum class RegionFormatType {
    SelectLanguage = 0,
    SelectKBLayout,
    SetupTimezone,
    SelectRegion,
    RegionFormat
};

class RegionFormatItemFrame : public DFrame
{
    Q_OBJECT
public:
    explicit RegionFormatItemFrame(QWidget *parent = nullptr);
    void initUI();
    void setIconPixmap(const QString &iconRes, const QSize &iconSize);
    void setItemType(RegionFormatType type);
    RegionFormatType getItemType();
    void setCheckState(bool checked);
    void setTitleText(const QString &text);
    void setDescriptionText(const QString &text);
    void initListData(const QStringList &combContents, const QStringList &combEnglishContents, const QString &defaultText);
    int getListDataIndex();
    QString getListDataText();
    void onComboBoxMenuSelected(int index);
    void setSearchableListViewShowPos(QPoint pos);
    void showSearchableListViewDialog();
    void hideSearchableListViewDialog();
    bool hasShowSearchableListViewDialog();

Q_SIGNALS:
    void popSearchableListViewDialog(int regionFormatType);
    void comboxChanged(int comboIndex);

private Q_SLOTS:
    void onCurrentListItemChanged(const QString &text);

protected:
    bool event(QEvent *e) override;

private:
    DLabel *m_title = nullptr;
    DLabel *m_description = nullptr;
    DLabel *m_icon = nullptr;
    DLabel *m_comboBoxText = nullptr;
    DIconButton* m_button = nullptr;
    SearchableListViewDialog *m_searchableListViewDialog = nullptr;
    RegionFormatType m_type = RegionFormatType::SelectLanguage;
    QPoint m_SearchableListViewShowPos;
};


#endif //REGIONFORMAT_ITEMFRAME_H
