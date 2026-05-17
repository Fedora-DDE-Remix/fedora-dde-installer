//
// Created by dtx on 2024/9/12.
//

#ifndef DEEPIN_INSTALLER_REGIONFORMATFRAME_H
#define DEEPIN_INSTALLER_REGIONFORMATFRAME_H

#include "FramePluginInterface.h"
#include "KeyboardLayoutManager.h"
#include "structs.h"
#include "RegionFormatItemFrame.h"

#include <DSuggestButton>
#include <QPushButton>
#include <QListWidget>
#include <QStyledItemDelegate>
#include <QPainterPath>

class TitleWidget;

class listItemDelegate : public QStyledItemDelegate
{
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        // 保存当前画家状态
        painter->save();

        // 创建一个带有圆角的路径
        QPainterPath path;
        path.addRoundedRect(option.rect, 5, 5);

        // 设置边框颜色和宽度
        QPen pen(QColor(240, 240, 240), 1);
        painter->setPen(pen);

        // 设置背景颜色
        QBrush brush(QColor(248, 248, 248));
        painter->fillPath(path, brush);

        // 绘制带有圆角的边框
        painter->drawPath(path);

        // 恢复画家状态
        painter->restore();

        // 调用基类的 paint 方法绘制 item 的文本和图标等内容
        QStyledItemDelegate::paint(painter, option, index);
    }
};

class RegionFormatFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)

public:
    explicit RegionFormatFrame(QWidget *parent = nullptr);
    ~RegionFormatFrame() override;

private:
    void initUI();
    void initConnection();
    void updateTs() override;
    void initListContent();
    void initLanguageItemContent(RegionFormatItemFrame &itemFrame);
    void initKBLayoutItemContent(RegionFormatItemFrame &itemFrame);
    void initTimezoneItemContent(RegionFormatItemFrame &itemFrame);
    void initRegionItemContent(RegionFormatItemFrame &itemFrame);
    void initRegionFormatItemContent(RegionFormatItemFrame &itemFrame);
    void localizeVariantList(const QString &locale);
    QString langRegionTrans(const QLocale &testLocale) const;
    QString langCountryTrans(const QLocale &testLocale, bool resetChinaName = false) const;
    bool updateVariantListByLocale(const QString& locale);
    void onLanguageMenuSelected(int index);
    void writeConf();
    void hideSearchableListViewDialog();
    std::optional<QStringList> getSupportedLocale();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onNextBtnClicked(bool checked = false);
    void onListItemClicked(QListWidgetItem *item);
    void onPopSearchableListViewDialog(int regionFormatType);

private:
    TitleWidget *m_title = nullptr;
    QListWidget *m_listWidget = nullptr;
    DSuggestButton *m_nextBnt = nullptr;

    LanguageList m_langItemList;
    LayoutVariantsMap m_layoutMap;
    XKbLayoutVariantList m_variantList;
    QMap<QString, QString> m_totalTimeZones;
    QMap<QString, QLocale> m_regionMap;
    QMap<QString, QLocale> m_regionFormatMap;
    listItemDelegate *m_listItemDelegate = nullptr;
    std::optional<QStringList> m_supportedLocaleList;
    QString m_locale = "";
};


#endif //DEEPIN_INSTALLER_REGIONFORMATFRAME_H
