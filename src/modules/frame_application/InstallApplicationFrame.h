#ifndef INSTALLDEBFRAME_H
#define INSTALLDEBFRAME_H

#include "FramePluginInterface.h"
#include "AnimeStackedWidget.h"
#include <QWidget>
#include <DSuggestButton>
#include <DButtonBox>
#include <QStackedWidget>
#include <QFileInfo>

class TitleWidget;

DWIDGET_USE_NAMESPACE

class InstallApplicationFrame: public FramePluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    explicit InstallApplicationFrame(QWidget *parent = nullptr);
    bool load() override;
protected:
    void showEvent(QShowEvent *event) override;
private:
    void updateTs() override;

    void initConnection();

    void addApplicationItem(QList<QFileInfo> &category);
private:
    TitleWidget* m_titleWidget;
    DButtonBox *m_btnBox;
    DSuggestButton *m_nextBnt;
    QStackedWidget *m_appStack;
};

#endif // INSTALLDEBFRAME_H
