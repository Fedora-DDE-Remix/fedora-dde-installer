#ifndef SELECTLANGUAGEFREAM_H
#define SELECTLANGUAGEFREAM_H

#include "FramePluginInterface.h"
#include "SelectLanguageWidget.h"
#include "LicenseDialog.h"

#include <QTranslator>
#include <DLabel>
#include <DCheckBox>
#include <DTextEdit>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

enum LicenseType {
    endUserLicenseAgreement,
    userExperienceAgreement,
    privacyPolicy
};

class QPushButton;
class SelectLanguageFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    explicit SelectLanguageFrame(QWidget *parent = nullptr);
    ~SelectLanguageFrame() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;


private slots:
//  点击下一步按钮后触发的槽函数，给配置文件写入相应的配置
    void writeConf();
//  动态更新当前窗口的翻译文案
    void updateTs() override;
//  点击返回按钮
    void onBackButton();
private:
//  显示用户协议
    void requestShowUserLicense();
    void requestShowUserExperience();
    void requestShowPrivacyLicense();
//  是否显示用户体验计划
    bool isShowExperienceWidget();
//  用户协议的标题
    QString userExperienceTitle();
    QString getLicenseText(LicenseType licenseType);

    bool checkDiskSpace();

    void setLabelText(DLabel *itemLabel, const QString &content, const int textWidth);

private:
    DSuggestButton *m_nextBnt = nullptr;
    QPushButton *m_backBut = nullptr;
    DLabel *m_title = nullptr;
    SelectLanguageWidget *m_selectLanguageWidget = nullptr;
    DCheckBox *m_acceptExperience = nullptr;
    DLabel *m_experienceLabel = nullptr;
    DCheckBox *m_acceptLicense = nullptr;
    DLabel *m_licenseLabel = nullptr;
    DLabel *m_andLabel = nullptr;
    DLabel *m_privacyLicenseLabel = nullptr;
    DLabel *m_versionLabel = nullptr;
    QFrame *m_licenseWidget = nullptr;
    QFrame *m_experienceWidget = nullptr;
    QTranslator *m_translator = nullptr;
};

#endif // SELECTLANGUAGEFREAM_H
