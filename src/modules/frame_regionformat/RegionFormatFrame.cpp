//
// Created by dtx on 2024/9/12.
//

#include "RegionFormatFrame.h"
#include "TitleWidget.h"
#include "CommonFunc.h"
#include "globals.h"
#include "TimezoneManager.h"

#include <QKeyEvent>
#include <libintl.h>
#include <QCollator>
#include <QLocale>
#include <QStandardPaths>
#include <QSettings>

#include <unicode/locid.h>
#include <unicode/unistr.h>

using icu::Locale;
using icu::UnicodeString;

const int ITEM_FRAME_HEIGHT = 105;
const int ITEM_FRAME_WIDTH = 635;
const int LIST_VIEW_HEIGHT = 500;
const int LIST_VIEW_WIDTH = 685;

RegionFormatFrame::RegionFormatFrame(QWidget *parent) : FramePluginInterface (parent)
{
    // 初始化数据部分
    m_langItemList = CommonFunc::getLanguageList();
    KeyboardLayoutManager::instance()->xkbLayoutMap(m_layoutMap);

    initUI();

    initConnection();

    initListContent();

    qApp->installEventFilter(this);
}

RegionFormatFrame::~RegionFormatFrame()
{

}

void RegionFormatFrame::initUI()
{
    m_title = new TitleWidget;

    m_listItemDelegate = new listItemDelegate();

    m_listWidget = new QListWidget(this);
    m_listWidget->setObjectName("SelectOSVersionFrameListWidget");
    m_listWidget->setFrameShape(QFrame::Shape::NoFrame);
    // 不能编辑触发
    m_listWidget->setEditTriggers(QListView::NoEditTriggers);
    // 不可以拖拽
    m_listWidget->setDragEnabled(false);
    m_listWidget->clearSelection();
    m_listWidget->clearFocus();
    m_listWidget->clearMask();
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_listWidget->setFixedWidth(LIST_VIEW_WIDTH);
    m_listWidget->setResizeMode(QListView::Fixed);
    m_listWidget->setViewMode(QListView::ListMode);
    m_listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_listWidget->setContentsMargins(0, 0, 0, 0);
    m_listWidget->setSpacing(10);
    // 设置其他属性
    m_listWidget->setTextElideMode(Qt::ElideRight);
    m_listWidget->setFocusPolicy(Qt::TabFocus);
    m_listWidget->setStyleSheet("QListWidget::item:focus{border:2px solid; border-radius:5px; border-color:rgb(1, 128, 255)}");
    m_listWidget->setItemDelegate(m_listItemDelegate);

    // 下一步
    m_nextBnt = new DSuggestButton;
    m_nextBnt->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBnt->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_nextBnt->setDefault(true);

    // 主布局
    m_contentLayout->addWidget(m_title);
    m_contentLayout->addSpacing(30);
    m_contentLayout->addWidget(m_listWidget, 0, Qt::AlignHCenter);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());
    m_contentLayout->addWidget(m_nextBnt, 0, Qt::AlignHCenter | Qt::AlignBottom);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());
}

void RegionFormatFrame::initConnection()
{
    connect(m_nextBnt, &DSuggestButton::clicked, this, &RegionFormatFrame::onNextBtnClicked);
    connect(m_listWidget, &QListWidget::itemClicked, this, &RegionFormatFrame::onListItemClicked);
}

void RegionFormatFrame::initListContent()
{
    //按钮  中间标题
    QList<QPair<RegionFormatType, QString>> typeIcons = {{RegionFormatType::SelectLanguage, ":/images/region_format_language.svg"},
                                                                       {RegionFormatType::SelectKBLayout, ":/images/region_format_kblayout.svg"},
                                                                       {RegionFormatType::SetupTimezone,  ":/images/region_format_timezone.svg"},
                                                                       {RegionFormatType::SelectRegion,   ":/images/region_format_area.svg"},
                                                                       {RegionFormatType::RegionFormat,   ":/images/region_format_format.svg"}};

    for (int i = 0; i < typeIcons.size(); i++) {
        RegionFormatItemFrame *itemFrame = new RegionFormatItemFrame();
        itemFrame->setItemType(typeIcons.at(i).first);
        itemFrame->setFixedWidth(ITEM_FRAME_WIDTH);
        itemFrame->setIconPixmap(typeIcons.at(i).second, QSize(32, 32));
        connect(itemFrame, &RegionFormatItemFrame::popSearchableListViewDialog, this, &RegionFormatFrame::onPopSearchableListViewDialog);
        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(ITEM_FRAME_WIDTH, ITEM_FRAME_HEIGHT));
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, itemFrame);

        if (typeIcons.at(i).first == RegionFormatType::SelectLanguage) {
            initLanguageItemContent(*itemFrame);
            connect(itemFrame, &RegionFormatItemFrame::comboxChanged, this, &RegionFormatFrame::onLanguageMenuSelected);
        } else if (typeIcons.at(i).first == RegionFormatType::SelectKBLayout) {
            initKBLayoutItemContent(*itemFrame);
        } else if (typeIcons.at(i).first == RegionFormatType::SetupTimezone) {
            initTimezoneItemContent(*itemFrame);
        } else if (typeIcons.at(i).first == RegionFormatType::SelectRegion) {
            initRegionItemContent(*itemFrame);
        } else if (typeIcons.at(i).first == RegionFormatType::RegionFormat) {
            initRegionFormatItemContent(*itemFrame);
        }

    }
}

void RegionFormatFrame::initLanguageItemContent(RegionFormatItemFrame &itemFrame)
{
    QString localeNameCode = CommonFunc::getSettingsString("DI_LOCALE");
    QStringList languageStringList;
    QStringList languageStringEnglishList;
    QString defaultSelecteText = "";
    for (auto languageItem : m_langItemList) {
        languageStringList.append(languageItem.localName);
        languageStringEnglishList.append(languageItem.name);
        if (languageItem.locale == localeNameCode) {
            defaultSelecteText = languageItem.localName;
        }
    }
    itemFrame.initListData(languageStringList, languageStringEnglishList, defaultSelecteText);
}

void RegionFormatFrame::initKBLayoutItemContent(RegionFormatItemFrame &itemFrame)
{
    QString localeNameCode = CommonFunc::getSettingsString("DI_LOCALE");
    QString layoutName = CommonFunc::getSettingsString("DI_LAYOUT");
    QString variantName = CommonFunc::getSettingsString("DI_LAYOUT_VARIANT");
    if (variantName.isEmpty()) {
        variantName = layoutName;
    }

    updateVariantListByLocale(m_locale);
    localizeVariantList(localeNameCode);
    QStringList descList;
    QStringList descEnglishList;
    QString defaultSelecteText = "";
    for (int i = 0; i < m_variantList.length(); ++i) {
        descList.append(m_variantList.at(i).description);
        descEnglishList.append(m_variantList.at(i).name);
        QStringList languageStringEnglishList;
        if (m_variantList.at(i).name == variantName) {
            defaultSelecteText = m_variantList.at(i).description;
        }
    }

    if (defaultSelecteText.isEmpty()) {
        if (descList.count() > 0) {
            defaultSelecteText = descList.first();
        }
    }

    itemFrame.initListData(descList, descEnglishList, defaultSelecteText);
}

void RegionFormatFrame::initTimezoneItemContent(RegionFormatItemFrame &itemFrame)
{
    QString localeNameCode(CommonFunc::getSettingsString("DI_LOCALE"));
    // 根据语言设置更新时区设置数据
    QString timezoneName(CommonFunc::getSettingsString("DI_TIMEZONE"));
    if (timezoneName.isEmpty()) {
        for (auto &it : m_langItemList) {
            if (it.locale == m_locale) {
                timezoneName = it.timezone;
                break;
            }
        }

        if (timezoneName.isEmpty()) {
            qWarning() << "Timezone is empty in all places";
            timezoneName = "Asia/Beijing";
        }
    }
    QStringList tms;
    QString localCity;

    TimezoneManager::instance()->getTotalTimezones(tms);
    QList<QPair<QString, QString>> localizedTms;

    for (const auto& it : tms) {
        QString key(TimezoneManager::instance()->getLocalTimezone(it, localeNameCode).second);
        localizedTms << QPair<QString, QString>(key, it);

        if (it == timezoneName) {
            localCity = key;
        }
    }

    // 按照locale排序
    QCollator collator{QLocale(m_locale)}; 
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort(localizedTms.begin(), localizedTms.end()
            , [&](const QPair<QString, QString>& a, const QPair<QString, QString>& b) {
                return collator.compare(a.first, b.first) < 0;
            }
    );

    // 所有时区的<本地化，英语名称>列表
    m_totalTimeZones.clear();
    for (const auto& it : localizedTms) {
        m_totalTimeZones.insert(it.first, it.second);
    }

    QStringList timeZoneList;
    QStringList timeZoneEnglishList;
    for (const QString &localName : m_totalTimeZones.keys()) {
        timeZoneList.append(localName);
        timeZoneEnglishList.append(m_totalTimeZones.value(localName));
    }

    itemFrame.initListData(timeZoneList, timeZoneEnglishList, localCity);
}

void RegionFormatFrame::initRegionItemContent(RegionFormatItemFrame &itemFrame)
{
    m_regionMap.clear();

    QList<QLocale> locales =
            QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    locales.removeOne(QLocale::C);
    // NOTE: sorry for Sichuang friends
    locales.removeOne(QLocale::SichuanYi);

    for (const auto &locale : locales) {
        QString script = locale.scriptToString(locale.script());
        QString language = locale.languageToString(locale.language());
        QString country = locale.territoryToString(locale.territory());
        // NOTE: sorry for guangdong friends
        if (locale.language() == QLocale::Cantonese && locale.territory() == QLocale::China) {
            continue;
        }
        // NOTE: the region `World` is weird
        if (locale.territory() == QLocale::Territory::World) {
            continue;
        }

        QString langCountryStr = langCountryTrans(locale);

        if (!m_regionMap.contains(langCountryStr) && !langCountryStr.isEmpty()) {
            m_regionMap.insert(langCountryStr, locale);
        }
    }

    QLocale regionFormatLocale = QLocale(CommonFunc::getSettingsString("DI_LOCALE"));

    QStringList countryList;
    QStringList countryEnglishList;

    for (const QString &itemStr : m_regionMap.keys()) {
        QLocale itemLocale = m_regionMap.value(itemStr);
        countryList.append(langCountryTrans(itemLocale));
        countryEnglishList.append(itemLocale.territoryToString(itemLocale.territory()));
    }

    QString langCountryStr = langCountryTrans(regionFormatLocale);
    itemFrame.initListData(countryList, countryEnglishList, langCountryStr);
}

void RegionFormatFrame::initRegionFormatItemContent(RegionFormatItemFrame &itemFrame)
{
    m_regionFormatMap.clear();

    QList<QLocale> locales =
            QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    locales.removeOne(QLocale::C);
    // NOTE: sorry for Sichuan friends
    locales.removeOne(QLocale::SichuanYi);

    QStringList regionFormatList;
    QStringList regionFormatEnglishList;

    for (const auto &locale : locales) {
        QString script = locale.scriptToString(locale.script());
        QString language = locale.languageToString(locale.language());
        QString country = locale.territoryToString(locale.territory());
        // NOTE: sorry for guangdong friends
        if (locale.language() == QLocale::Cantonese && locale.territory() == QLocale::China) {
            continue;
        }
        // NOTE: the region `World` is weird
        if (locale.territory() == QLocale::Territory::World) {
            continue;
        }

        QString langRegionStr = langRegionTrans(locale);
        if (!regionFormatList.contains(langRegionStr)) {
            regionFormatList.append(langRegionStr);
        }

        QString langRegionEnglishStr = QString("%1:%2").arg(language).arg(country);
        if (!regionFormatEnglishList.contains(langRegionEnglishStr)) {
            regionFormatEnglishList.append(langRegionEnglishStr);
        }

        if (!m_regionFormatMap.contains(langRegionStr)) {
            m_regionFormatMap.insert(langRegionStr, locale);
        }
    }

    QLocale regionFormatLocale = QLocale(CommonFunc::getSettingsString("DI_LOCALE"));

    QString defaultLangRegionStr = langRegionTrans(regionFormatLocale);
    itemFrame.initListData(regionFormatList, regionFormatEnglishList, defaultLangRegionStr);
}

QString RegionFormatFrame::langRegionTrans(const QLocale &testLocale) const
{
    QString localeNameCode(CommonFunc::getSettingsString("DI_LOCALE"));

    auto localeSystem = QLocale(localeNameCode);
    auto systemLocale = icu::Locale(localeSystem.name().toStdString().data());
    auto IcuLocale = Locale(testLocale.name().toStdString().data());
    auto localeHex = UnicodeString(testLocale.name().toStdString().data());
    std::string displayLanguageIcu;
    IcuLocale.getDisplayLanguage(systemLocale, localeHex).toUTF8String(displayLanguageIcu);
    std::string displayCountryIcu;
    IcuLocale.getDisplayCountry(systemLocale, localeHex).toUTF8String(displayCountryIcu);

    QLocale locale = testLocale;
    QString language = locale.languageToString(locale.language());
    QString country = locale.territoryToString(locale.territory());
    if ((locale.territory() == QLocale::HongKong || locale.territory() == QLocale::Taiwan)
        && locale.language() == QLocale::Chinese)
        language = "Traditional Chinese";
    if (locale.territory() == QLocale::China && locale.language() == QLocale::Chinese)
        language = "Simplified Chinese";
    QString langCountry = QString("%1(%2)").arg(displayLanguageIcu.c_str()).arg(langCountryTrans(testLocale, true));
    return langCountry;
}

QString RegionFormatFrame::langCountryTrans(const QLocale &testLocale, bool resetChinaName) const
{
    QString localeNameCode(CommonFunc::getSettingsString("DI_LOCALE"));

    auto localeSystem = QLocale(localeNameCode);
    auto systemLocale = icu::Locale(localeSystem.name().toStdString().data());
    auto IcuLocale = icu::Locale(testLocale.name().toStdString().data());
    auto localeHex = icu::UnicodeString(testLocale.name().toStdString().data());
    std::string displayCountryIcu;
    IcuLocale.getDisplayCountry(systemLocale, localeHex).toUTF8String(displayCountryIcu);

    QString displayCountry = QString::fromStdString(displayCountryIcu);
    if (displayCountry == "台湾") {
        displayCountry = "中国台湾";
    } else if (displayCountry == "台灣") {
        displayCountry = "中國台灣";
    } else if (displayCountry == "臺灣") {
        displayCountry = "中國臺灣";
    } else if (displayCountry == "Taiwan") {
        displayCountry = "Taiwan China";
    } else if (displayCountry == "中国" && resetChinaName) {
        displayCountry = "中国大陆";
    } else if (displayCountry == "中國" && resetChinaName) {
        displayCountry = "中國大陸";
    } else if (displayCountry == "China" && resetChinaName) {
        displayCountry = "Mainland China";
    }
    return displayCountry;
}

void RegionFormatFrame::localizeVariantList(const QString &locale)
{
    QString oldLocale = setlocale(LC_ALL, nullptr);
    qInfo() << QString("Before localized keyboard layout, old locale is %1").arg(oldLocale);

    (void) setlocale(LC_ALL, QString(locale).append(".UTF-8").toStdString().c_str());

    const char xkbDomain[] = "xkeyboard-config";
    for (auto &iter : m_variantList) {
        iter.description = dgettext(xkbDomain, iter.description.toLocal8Bit().constData());
    }

    (void) setlocale(LC_ALL, oldLocale.toStdString().c_str());
}

bool RegionFormatFrame::updateVariantListByLocale(const QString& locale)
{
    QString layoutName(CommonFunc::get2AlphaCountryCode(locale).toLower());
    m_variantList = m_layoutMap.value("us");

    // 如果所选的语言没有对应的键盘，那么键盘列表不发生改变
    if (m_layoutMap.contains(layoutName)) {
        m_variantList = m_layoutMap.value(layoutName);
    } else {
        if (!layoutName.compare("hk", Qt::CaseInsensitive) || !layoutName.compare("tw", Qt::CaseInsensitive)) {
            m_variantList = m_layoutMap.value("cn");
        }
    }

    return !m_variantList.isEmpty();
}

void RegionFormatFrame::onLanguageMenuSelected(int index)
{
    if (0 <= index && index < m_langItemList.length()) {
        const QString locale = m_langItemList.at(index).locale;

        m_locale = locale;

        int rowCount = m_listWidget->count();
        for (int i = 0; i < rowCount; ++i) {
            RegionFormatItemFrame *item = qobject_cast<RegionFormatItemFrame *>(m_listWidget->itemWidget(m_listWidget->item(i)));
            if (item->getItemType() == RegionFormatType::SelectKBLayout) {
                initKBLayoutItemContent(*item);
            } else if (item->getItemType() == RegionFormatType::SetupTimezone) {
                initTimezoneItemContent(*item);
            } else if (item->getItemType() == RegionFormatType::SelectRegion) {
                initRegionItemContent(*item);
            } else if (item->getItemType() == RegionFormatType::RegionFormat) {
                initRegionFormatItemContent(*item);
            }
        }
    }
}

void RegionFormatFrame::writeConf()
{
    QString currentLanguage = "";
    int currentVariantIndex = 0;
    QString currentTimeZone = "";
    QLocale currentRegionLocale;
    QLocale currentRegionFormatLocale;

    // 获取各项目当前值
    int rowCount = m_listWidget->count();
    for (int i = 0; i < rowCount; ++i) {
        RegionFormatItemFrame *item = qobject_cast<RegionFormatItemFrame *>(m_listWidget->itemWidget(m_listWidget->item(i)));
        if (item->getItemType() == RegionFormatType::SelectLanguage) {
            currentLanguage = item->getListDataText();
        } else if (item->getItemType() == RegionFormatType::SelectKBLayout) {
            currentVariantIndex = item->getListDataIndex();
        } else if (item->getItemType() == RegionFormatType::SetupTimezone) {
            currentTimeZone = m_totalTimeZones.value(item->getListDataText());
        } else if (item->getItemType() == RegionFormatType::SelectRegion) {
            currentRegionLocale = m_regionMap.value(item->getListDataText());
        } else if (item->getItemType() == RegionFormatType::RegionFormat) {
            currentRegionFormatLocale = m_regionFormatMap.value(item->getListDataText());
        }
    }

    // 保存语言配置
    QString localeNameCode = "zh_CN";
    LanguageList::iterator resultLangIter = std::find_if(m_langItemList.begin(), m_langItemList.end(), [=](const SLanguageItem &langItem){
        return (langItem.localName == currentLanguage);
    });

    if (resultLangIter != m_langItemList.end()) {
        localeNameCode = (*resultLangIter).locale;
    }

    CommonFunc::setSettingsValue("DI_LOCALE", localeNameCode);


    // 保存键盘布局配置
    QString layoutName(CommonFunc::get2AlphaCountryCode(localeNameCode).toLower());

    if (!m_layoutMap.contains(layoutName)) {
        qWarning() << QString("There is no keyboard setting for locale %1").arg(localeNameCode);
    } else {
        m_variantList = m_layoutMap.value(layoutName);

        if (m_variantList.isEmpty()) {
            qWarning() << QString("Thers is not variant in layout:%1").arg(layoutName);
        }

        QString variant = m_variantList.at(currentVariantIndex).name;
        if (layoutName == variant) {
            variant.clear();
        }

        CommonFunc::setSettingsValue("DI_LAYOUT", layoutName);
        CommonFunc::setSettingsValue("DI_LAYOUT_VARIANT", variant);
    }


    // 保存时区配置
    if (!TimezoneManager::instance()->isTimezoneInTab(currentTimeZone)) {
        qWarning() << QString("Invalid timezone %1 in config").arg(currentTimeZone);
        return;
    }

    const ZoneInfoList zoneinfoList = TimezoneManager::instance()->getZoneInfoList();
    const SZoneInfo curZoneInfo = zoneinfoList[TimezoneManager::instance()->getZoneInfoIndexByTimezone(zoneinfoList, currentTimeZone)];
    QString localtime;
    if (curZoneInfo.country == "CN" && curZoneInfo.timezone != "Asia/Urumqi") {
        localtime = "Asia/Shanghai";
    } else {
        localtime = currentTimeZone;
    }

    CommonFunc::setSettingsValue("DI_TIMEZONE", currentTimeZone);
    CommonFunc::setSettingsValue("DI_LOCALTIME", localtime);

    // 保存区域格式配置
    //将被设置到org.deepin.region-format中country的值
    QString countryStr = currentRegionLocale.territoryToString(currentRegionLocale.territory());
    CommonFunc::setSettingsValue("DI_REGION", countryStr);

    //将被设置到org.deepin.region-format中languageRegion的值
    QString language = currentRegionFormatLocale.languageToString(currentRegionFormatLocale.language());
    QString country = currentRegionFormatLocale.territoryToString(currentRegionFormatLocale.territory());
    if ((currentRegionFormatLocale.territory() == QLocale::HongKong || currentRegionFormatLocale.territory() == QLocale::Taiwan)
        && currentRegionFormatLocale.language() == QLocale::Chinese) {
        language = "Traditional Chinese";
    } else if (currentRegionFormatLocale.territory() == QLocale::China && currentRegionFormatLocale.language() == QLocale::Chinese) {
        language = "Simplified Chinese";
    }
    QString langRegionFormatStr = QString("%1:%2").arg(language).arg(country);
    CommonFunc::setSettingsValue("DI_REGION_FORMAT", langRegionFormatStr);

    //将被设置到org.deepin.region-format中localeName的值
    CommonFunc::setSettingsValue("DI_REGION_FORMAT_LOCALE" , currentRegionFormatLocale.name());

    //将被设置到locale.conf中的值
    auto supportedLocaleListRes = getSupportedLocale();
    QString localeSet = "";
    if (supportedLocaleListRes.has_value()) {
        QStringList supportedLocaleList = supportedLocaleListRes.value();
        if (QString otherLocaleName = currentRegionFormatLocale.name() + ".UTF-8";
            supportedLocaleList.contains(otherLocaleName)) {
            localeSet = otherLocaleName;
        } else if (supportedLocaleList.contains(currentRegionFormatLocale.name())) {
            localeSet = currentRegionFormatLocale.name();
        }
    }

    CommonFunc::setSettingsValue("DI_REGION_FORMAT_LOCALE_SET", localeSet);
}


void RegionFormatFrame::hideSearchableListViewDialog()
{
    int rowCount = m_listWidget->count();
    for (int i = 0; i < rowCount; ++i) {
        RegionFormatItemFrame *itemFrame = qobject_cast<RegionFormatItemFrame *>(m_listWidget->itemWidget(m_listWidget->item(i)));
        itemFrame->hideSearchableListViewDialog();
    }
}

std::optional<QStringList> RegionFormatFrame::getSupportedLocale()
{
    if (m_supportedLocaleList.has_value()) {
        return m_supportedLocaleList;
    }
    static QString LOCALE_LIST_FILE = QStringLiteral("/usr/share/i18n/SUPPORTED");
    QFile file(LOCALE_LIST_FILE);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return std::nullopt;
    QStringList localelist;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList out = line.split(" ");
        localelist.push_back(out[0]);
    }
    m_supportedLocaleList = localelist;
    return m_supportedLocaleList;
}

bool RegionFormatFrame::eventFilter(QObject *watched, QEvent *event)
{
   if (this->isHidden()) {
       return FramePluginInterface::eventFilter(watched, event);
   }

   if (watched->objectName() == "SelectOSVersionFrameListWidget") {
       if (event->type() == QEvent::KeyPress) {
           QKeyEvent *key = dynamic_cast<QKeyEvent*>(event);
           if (key != nullptr) {
               int key_value = key->key();
               switch (key_value) {
                   case Qt::Key_Return: {
                       if (m_listWidget->currentIndex().isValid()) {
                           Q_EMIT m_listWidget->itemClicked(m_listWidget->currentItem());
                       }
                   }
                   default: qInfo() << "key pressed not thing to do";
               }
           }
       } else if (event->type() == QEvent::MouseButtonPress) {
           QMouseEvent *mouse = dynamic_cast<QMouseEvent*>(event);
           if (mouse != nullptr) {
               if (mouse->button() == Qt::LeftButton) {
                   if (m_listWidget->currentIndex().isValid()) {
                       Q_EMIT m_listWidget->itemClicked(m_listWidget->currentItem());
                   }
               }
           }
       }
   } else if (watched->objectName() == "SearchableListViewDialog") {
       if (event->type() == QEvent::WindowDeactivate) {
            hideSearchableListViewDialog();
       }
   }

    return FramePluginInterface::eventFilter(watched, event);
}

void RegionFormatFrame::showEvent(QShowEvent *event)
{
    int rowCount = m_listWidget->count();
    for (int i = 0; i < rowCount; ++i) {
        RegionFormatItemFrame *itemFrame = qobject_cast<RegionFormatItemFrame *>(m_listWidget->itemWidget(m_listWidget->item(i)));
        itemFrame->setSearchableListViewShowPos(QPoint(itemFrame->pos().x(), 0));
    }

    FramePluginInterface::showEvent(event);
}

void RegionFormatFrame::onNextBtnClicked(bool checked)
{
    hideSearchableListViewDialog();
    writeConf();
    Q_EMIT next();
}

void RegionFormatFrame::onListItemClicked(QListWidgetItem *item)
{
    hideSearchableListViewDialog();

    if (item != nullptr) {
        RegionFormatItemFrame *itemFrame = qobject_cast<RegionFormatItemFrame *>(m_listWidget->itemWidget(item));
        itemFrame->showSearchableListViewDialog();
    }
}

void RegionFormatFrame::onPopSearchableListViewDialog(int regionFormatType)
{
    int rowCount = m_listWidget->count();
    for (int i = 0; i < rowCount; ++i) {
        RegionFormatItemFrame *item = qobject_cast<RegionFormatItemFrame *>(m_listWidget->itemWidget(m_listWidget->item(i)));
        if (static_cast<int>(item->getItemType()) == regionFormatType) {
            Q_EMIT m_listWidget->itemClicked(m_listWidget->item(i));
            break;
        }
    }
}

void RegionFormatFrame::updateTs()
{
    m_locale = CommonFunc::getSettingsString("DI_LOCALE");

    m_title->setTitleLabelProperty(::QObject::tr("System settings"));
    m_title->setTipsLabelProperty(::QObject::tr("Set the system language, keyboard layout, time zone and region"));
    m_nextBnt->setText(::QObject::tr("Next"));

    int rowCount = m_listWidget->count();
    int itemHeight = 0;
    for (int i = 0; i < rowCount; ++i) {
        RegionFormatItemFrame *item = qobject_cast<RegionFormatItemFrame *>(m_listWidget->itemWidget(m_listWidget->item(i)));
        if (item->getItemType() == RegionFormatType::SelectLanguage) {
            item->setTitleText(::QObject::tr("Language"));
            item->setDescriptionText(::QObject::tr("Control Center, File Manager and other system interfaces will be displayed in this language"));
            initLanguageItemContent(*item);
        } else if (item->getItemType() == RegionFormatType::SelectKBLayout) {
            item->setTitleText(::QObject::tr("Keyboard layout"));
            item->setDescriptionText(::QObject::tr("Select your keyboard layout, which will be used as the system default"));
            initKBLayoutItemContent(*item);
        } else if (item->getItemType() == RegionFormatType::SetupTimezone) {
            item->setTitleText(::QObject::tr("Time zone"));
            item->setDescriptionText(::QObject::tr("Select your time zone, it will be the system default"));
            initTimezoneItemContent(*item);
        } else if (item->getItemType() == RegionFormatType::SelectRegion) {
            item->setTitleText(::QObject::tr("Region"));
            item->setDescriptionText(::QObject::tr("The operating system and app store will provide you with local content based on your country or region"));
            initRegionItemContent(*item);
        } else if (item->getItemType() == RegionFormatType::RegionFormat) {
            item->setTitleText(::QObject::tr("Regional formats"));
            item->setDescriptionText(::QObject::tr("The operating system uses your chosen region settings to determine localized formats"));
            initRegionFormatItemContent(*item);
        }

        item->adjustSize();
        if (item->height() > itemHeight) {
            itemHeight = item->height();
        }
    }

    for (int i = 0; i < rowCount; ++i) {
        m_listWidget->item(i)->setSizeHint(QSize(ITEM_FRAME_WIDTH, itemHeight));
    }
}