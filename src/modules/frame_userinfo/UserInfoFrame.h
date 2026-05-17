#ifndef USERINFOFRAME_H
#define USERINFOFRAME_H

#include "FramePluginInterface.h"
#include "UserInfoWidget.h"
#include "AvatarListWidget.h"
#include "PopupMenu.h"
#include "structs.h"
#include "KeyboardLayoutManager.h"
#include "TimezoneFrame.h"
#include "IconComboBox.h"
#include "PopupWindow.h"
#include "tray_module_interface.h"

#include <DLineEdit>
#include <DPasswordEdit>
#include <DSuggestButton>


DWIDGET_USE_NAMESPACE

class QPushButton;
class QLabel;
class AvatarButton;
class TitleWidget;


class UserInfoFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)

public:
    explicit UserInfoFrame(QWidget *parent = nullptr);

    bool load() override;

signals:
    void timezoneUpdated(const QString& timezone);

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void initUI();
    void updateTs() override;

    void writeConf();

    void initKeyboard();


    void initLanguage();


    void onNextButtonClicked();

    void localizeVariantList(const QString &locale);
    bool updateVariantListByLocale(const QString& locale);

    void updateKeyboardLayoutConf(const QString &locale);

    void updateTimezoneConf(const QString &locale);

    bool searchDevice();

private slots:
    void onKeyboardMenuSelected(int index);
    void onLanguageMenuSelected(int index);
    void addModule(dss::module::BaseModuleInterface *module);
private:

    TitleWidget* m_titleWidget = nullptr;

    AvatarButton* m_currentAvatarButton = nullptr;
    AvatarListWidget* m_avatarListWidget = nullptr;

    UserInfoWidget* m_userInfoWidget = nullptr;

    DCheckBox *m_quickLogin = nullptr;

    IconComboBox *m_cbKeyboard = nullptr;
    IconComboBox *m_cbLanguage = nullptr;

    DIconButton *m_timezoneBtn = nullptr;

    DIconButton *m_networkBtn = nullptr;

    PopupWindow *m_networkWidget = nullptr;

    dss::module::TrayModuleInterface *m_networkModule = nullptr;

    DSuggestButton *m_nextBnt = nullptr;

    TimezoneFrame* m_timezone = nullptr;

    LanguageList m_langItemList;

    LayoutVariantsMap m_layoutMap;
    XKbLayoutVariantList m_variantList;
};

#endif
