#ifndef DEBUGFRAME_H
#define DEBUGFRAME_H

#include <qtermwidget6/qtermwidget.h>

#include "FramePluginInterface.h"

class DebugShellFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    explicit DebugShellFrame(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QTermWidget* m_termWidget = nullptr;
    QHBoxLayout* m_mainLayout = nullptr;

};

#endif // PROGRESSFRAME_H
