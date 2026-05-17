#ifndef INSTALLPREPARE_H
#define INSTALLPREPARE_H
#include "FramePluginInterface.h"
#include "TitleWidget.h"
#include <DTipLabel>
#include <DSuggestButton>
#include <QCheckBox>
#include <QScrollArea>


DWIDGET_USE_NAMESPACE

class InstallPrepare:public FramePluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    explicit InstallPrepare(QWidget *parent = nullptr);
protected:
    void showEvent(QShowEvent *event) override;
private:
    void initUI();

    void initConnection();

    void updateTs() override;

private:
    TitleWidget *m_title = nullptr;
    DLabel *m_labelInfo = nullptr;
    DSuggestButton *m_btnNext = nullptr;
    QCheckBox *m_nvidiaDriver = nullptr;
};

#endif // INSTALLPREPARE_H
