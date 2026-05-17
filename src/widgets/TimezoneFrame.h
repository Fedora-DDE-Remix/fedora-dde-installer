#ifndef TIMEZONEFRAME_H
#define TIMEZONEFRAME_H

#include "FramePluginInterface.h"
#include "TimezoneMap.h"
#include "SelectTimeZoneFrame.h"

#include <DLabel>
#include <DTipLabel>
#include <DPushButton>
#include <DDialog>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class TimezoneFrame : public DDialog {
    Q_OBJECT

public:
    explicit TimezoneFrame(QWidget* parent = nullptr);

    static bool isTimezoneSetByUser();

protected:
    void showEvent(QShowEvent* event) override;

private:
    void readConf();
    void writeConf();

    void updateTs();

    void onTimezoneMapUpdated(const QString& timezone);
    void onResetButtonClicked();
    void onSelectTimezoneUpdated(const QString& timezone);

    QString getTimezoneAlias(const QString& timezone);

    // 时区数据为用户配置（来自配置文件或者用户GUI操作）
    static bool m_isConfSetByUser;

    QString m_timezone;
    // 备份进入页面时的timezone
    QString m_timezoneBak;

    DLabel *m_titleLabel;
    DTipLabel *m_msgLabel;
    TimezoneMap* m_timezoneMap = nullptr;
    SelectTimeZoneFrame* m_selectTimeZoneFrame = nullptr;

    QAbstractButton *m_okBnt = nullptr;
    QAbstractButton *m_cancelBnt = nullptr;

    TimezoneAliasMap m_aliasMap;
};

#endif // TIMEZONEFRAME_H
