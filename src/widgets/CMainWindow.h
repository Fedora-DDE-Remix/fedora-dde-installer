#include "structs.h"

#include <QFrame>
#include <DPageIndicator>
#include <QStack>
#include <DMainWindow>
#include <DPushButton>
#include <QLabel>
#include <QTranslator>

class QPushButton;
class ListWidget;
class QStackedLayout;
class FramePluginInterface;
class PageIndicator;

class  MainWindow : public QMainWindow//Dtk::Widget::DMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    // 设置主界面插件
    void showPlugins(const QStack<SPluginInfo> &plugs);

    void loadPlugins(const QString &pluginType = "deepin_installer_plugins");

    void setTranslator(QTranslator *trans) {
        m_translator = trans;
    }

    // 初始化主界面默认ui
    void initUI();

    void setPluginSize();

    bool eventFilter(QObject *watched, QEvent *event) override;
protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;
private slots:
    void initTranslator(const QString &locale);
private:
    void initPluginConnection(FramePluginInterface *frame, QStackedWidget *pluginWidget);

    void plugins(QStack<SPluginInfo> &lists);

    void debugShell();

    // 处理主界面的翻译
    void updateTs();
private:
    QLabel *m_titleLabel;
    QStackedWidget *m_pluginWidget;
    PageIndicator *m_pageWidgets;
    ListWidget *listWidget = nullptr;
    QStack<SPluginInfo> backLists;
    QStack<SPluginInfo> nextLists;
    QTranslator *m_translator;
    QWidget *m_framePartition;
    QWidget *m_framePrepare;
    QWidget *m_frameApplication;
    FramePluginInterface *m_debugShellFrame = nullptr;
};
