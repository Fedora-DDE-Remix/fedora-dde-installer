#pragma once

#include "FramePluginInterface.h"
#include "SelectKBLayoutWidget.h"
#include "TitleWidget.h"

#include <DLabel>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class QPushButton;
class SelectKBLayoutFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    explicit SelectKBLayoutFrame(QWidget *parent = nullptr);
    ~SelectKBLayoutFrame() override;

protected:
    void updateTs() override;

private:
    DSuggestButton *m_nextBnt = nullptr;
    QPushButton *m_backBut = nullptr;
    TitleWidget *m_title;
    //DLabel *m_title = nullptr;
    SelectKBLayoutWidget *m_selectKBLayoutWidget = nullptr;
};
