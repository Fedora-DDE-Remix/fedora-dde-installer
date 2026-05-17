#ifndef PASSWDDIALOG_H
#define PASSWDDIALOG_H

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DPasswordEdit>
#include <DSuggestButton>
#include <QStackedWidget>
#include <QTimer>
#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include "HelpDialog.h"

DWIDGET_BEGIN_NAMESPACE
class DTipLabel;
DWIDGET_END_NAMESPACE

DWIDGET_USE_NAMESPACE

class TitleWidget;
class PasswdDialog : public DDialog
{
    Q_OBJECT
public:
    explicit PasswdDialog(QWidget *parent = Q_NULLPTR);
    QString getPasswd();

    void setAutoDecryptVisible(bool isVisible);
    bool autoDecryptIsChecked();
private:
    void InitPage();

    void InitPasswordPage(QWidget *passwdPage);

    void InitExportPage(QWidget *exportPage);

    void initConnection();

    void removeOldDevice(const QString &id);

    void addUsbDevice(const QString &devName, const QString &devPath, const QString &devId, const QString &mountPath);
private Q_SLOTS:
    void checkPassword();

    void refreshUsbDevice();

    void exportKey();
private:
    DPasswordEdit *m_safeKeyEdit = Q_NULLPTR;
    DPasswordEdit *m_ensureEdit = Q_NULLPTR;
    DLabel *m_tipsLabel = Q_NULLPTR;
    DCheckBox *m_autoDecrypt = Q_NULLPTR;
    QAbstractButton *m_cancelBtn = Q_NULLPTR;
    QAbstractButton *m_nextBtn = Q_NULLPTR;
    AutoDecryptDialog *m_autoDecryptDlg;
    QString m_strPasswd = Q_NULLPTR;
    QStackedWidget *m_mainWidget;
    DLabel *m_keyLabel;
    DTipLabel *m_eptipLabel;
    QListView *m_listView;
    QStandardItemModel *m_model;
    QStringList m_devList;
    QTimer *m_timer;
    QWidget *m_decryptWidget = Q_NULLPTR;
};

#endif // PASSWDDIALOG_H
