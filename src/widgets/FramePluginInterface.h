#ifndef FRAMEPLUGININTERFACE_H
#define FRAMEPLUGININTERFACE_H

#include "PluginInterface.h"
#include "PopupWidget.h"

#include <QObject>
#include <DFrame>
#include <DIconButton>
#include <DWindowCloseButton>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

class QTranslator;

extern const QString gFrameLanguage;
extern const QString gFrameVerify;
extern const QString gFramePartition;
extern const QString gFrameUserInfo;
extern const QString gFramePrepare;
extern const QString gFrameInstall;
extern const QString gFrameConfigure;
extern const QString gFrameApplication;

class FramePluginInterface : public QWidget, public PluginInterface
{
    Q_OBJECT
    Q_PROPERTY(QString frameName READ frameName)
public:
    explicit FramePluginInterface(QWidget *parent = nullptr);

    void hideBackButton();

    void hideCloseButton();

    QString frameName();
signals:
    void next();
    void back();

    void onChangeLanguage(const QString &strLocale);

    void insertTempPage(FramePluginInterface *page);
protected:
    void quitInstaller();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

    virtual void updateTs();
private:
    void initTitleButton();
protected:
    QVBoxLayout *m_contentLayout;
    int m_index;
    QString m_frameName;
    DIconButton  *m_backBnt = nullptr;
    DWindowCloseButton  *m_closeBnt = nullptr;
};

#endif // FRAMEPLUGININTERFACE_H
