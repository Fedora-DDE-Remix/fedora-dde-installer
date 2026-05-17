#ifndef SELECTCOMPONENTFRAME_H
#define SELECTCOMPONENTFRAME_H

#include "FramePluginInterface.h"
#include <DSuggestButton>

class QListWidget;
class QListWidgetItem;
class QCheckBox;
class QPushButton;
class QButtonGroup;
class QLabel;
class ComponentsManager;
class TitleWidget;

class SelectComponentFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)

public:
    explicit SelectComponentFrame(QWidget *parent = nullptr);

public:
    bool load() override;

private slots:
    void nextButtonClickedSlot();
    void baseComponentListWidgetClickedSlot(QListWidgetItem *item);
    void extraComponentListWidgetClickedSlot(QListWidgetItem *item);
    void selectAllBoxClickedSlot();

private:
    void initUI();
    void initConnect();
    void updateTs() override;

protected:
    void showEvent(QShowEvent *event) override;

private:
    TitleWidget* m_titleWidget = nullptr;
    QLabel* m_leftTitleLabel = nullptr;
    QLabel* m_rightTitleLabel = nullptr;
    QPushButton* m_nextBnt = nullptr;
    QButtonGroup* m_buttonGroup = nullptr;
    QListWidget* m_baseComponentListWidget = nullptr;
    QListWidget* m_extraComponentListWidget = nullptr;
    QCheckBox *m_selectAllBox = nullptr;
    ComponentsManager* m_componentsManager = nullptr;

private:
    void setLeftList(const QStringList &listinfo);
    void setRightList(const QStringList &listinfo);
    bool checkAllExtraSelected();
    void saveData();
    QStringList getSlectList();
    void setSlectList(const QStringList &selectlist);
};

#endif // SELECTCOMPONENTFRAME_H
