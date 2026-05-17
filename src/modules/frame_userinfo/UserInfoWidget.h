#ifndef USERINFOWIDGET_H
#define USERINFOWIDGET_H

#include "ScrollAreaWidget.h"
#include "WrapWidget.h"

#include <QScrollArea>
#include <DLineEdit>
#include <DPasswordEdit>
#include <DComboBox>

DWIDGET_USE_NAMESPACE

class QLabel;
class QCheckBox;
class SystemInfoTip;

class UserInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UserInfoWidget(QWidget *partent = nullptr);

    void updateTs();
    void readConf();
    void writeConf();

    // 当lineEdit处于编辑态时，隐藏错误提示tip。
    // 检查下一步按钮的状态，并报告给主窗口。
    void checkNextButtonEnable();

    bool checkAllEditText();

signals:
    void nextButtonEnable(bool enable);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void initUI();

    void onUsernameTextChanged();
    void onHostnameTextChanged();
    void onUsernameEdited();
    void onHostnameEdited();


    // 校验lineEdit中的内容，如果失败，返回false，同时返回具体的错误信息。
    bool validateUsername(const QString& username, QString& msg);
    bool validateHostname(const QString& hostname, QString& msg);
    bool validatePassword(const QString &user, const QString &passwd, QString& msg);
    bool validateCheckPassword(const QString &passwd, const QString &checkPasswd, QString& msg);

    void updatePasswdLevel();
    void updateRootPasswdLevel();

    void updateCapsLockState(bool lock);
    void updateEditCapsLockState(DLineEdit* edit, bool lock);

    QLabel* m_usernameLabel = nullptr;
    DLineEdit* m_usernameEdit = nullptr;
    DComboBox* m_usernameComboBox = nullptr;
    QLabel* m_usernameReuseTip = nullptr;
    QLabel* m_usernameCheckTipPadding = nullptr;


    QLabel* m_hostnameLabel = nullptr;
    DLineEdit* m_hostnameEdit = nullptr;

    QLabel* m_passwordLabel = nullptr;
    DPasswordEdit* m_passwordEdit = nullptr;

    QLabel* m_passwdLevelLabel = nullptr;
    QLabel* m_rootPasswdLevelLabel = nullptr;

    QLabel* m_passwordCheckLabel = nullptr;
    DPasswordEdit* m_passwordCheckEdit = nullptr;

    WrapWidget *m_wrapWidget = nullptr;

    QCheckBox* m_setRootPasswordCheck = nullptr;

    QLabel *m_rootUserLabel = nullptr;
    QLabel* m_rootPasswordLabel = nullptr;
    DPasswordEdit* m_rootPasswordEdit = nullptr;
    QLabel* m_rootPasswordCheckLabel = nullptr;
    DPasswordEdit* m_rootPasswordCheckEdit = nullptr;

    QAction *m_capsLock = nullptr;

    // 如果主机名被手工编辑过，那么不再基于当前用户名合成主机名。
    bool m_isHostnameEditedManually = false;
};

#endif
