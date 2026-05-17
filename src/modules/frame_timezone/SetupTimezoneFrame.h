#pragma once

#include "FramePluginInterface.h"
#include "TimezoneMap.h"
#include "SelectTimeZoneFrame.h"
#include "TitleWidget.h"

#include <DLabel>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class QPushButton;
class SetupTimezoneFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    explicit SetupTimezoneFrame(QWidget *parent = nullptr);
    ~SetupTimezoneFrame() override;
    bool isConfSetByUser() { return m_isConfSetByUser; }

protected:
    void updateTs() override;
    void showEvent(QShowEvent *event) override;

private:
    void onResetButtonClicked();
    void initTimezoneConf(const QString &locale);
    void writeConf();

    DSuggestButton *m_nextBnt = nullptr;
    QPushButton *m_backBut = nullptr;

    // 时区数据为用户配置（来自配置文件或者用户GUI操作）
    bool m_isConfSetByUser;

    QString m_timezone;
    // 备份进入页面时的timezone
    QString m_timezoneBak;

    TitleWidget *m_title;

    TimezoneMap* m_timezoneMap = nullptr;
    SelectTimeZoneFrame* m_selectTimeZoneFrame = nullptr;

    QAbstractButton *m_okBnt = nullptr;
    QAbstractButton *m_cancelBnt = nullptr;

    TimezoneAliasMap m_aliasMap;
};
