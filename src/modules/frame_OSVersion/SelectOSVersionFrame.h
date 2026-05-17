//
// Created by dtx on 2024/8/22.
//

#ifndef DEEPIN_INSTALLER_SELECTOSVERSIONFRAME_H
#define DEEPIN_INSTALLER_SELECTOSVERSIONFRAME_H

#include "FramePluginInterface.h"

#include <DSuggestButton>
#include <QPushButton>
#include <DListView>
#include <QStandardItemModel>

class TitleWidget;
class SelectOSVersionFrame : public FramePluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)

public:
    explicit SelectOSVersionFrame(QWidget *parent = nullptr);
    ~SelectOSVersionFrame() override;
    virtual bool load() override;

private:
    void initUI();
    void initConnection();
    void updateTs() override;
    void initListContent();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onItemClicked(const QModelIndex &index);

private:
    TitleWidget *m_title = nullptr;
    DListView *m_OSVersionListView = nullptr;
    QStandardItemModel *m_model = nullptr;
    DSuggestButton *m_nextBnt = nullptr;
};


#endif //DEEPIN_INSTALLER_SELECTOSVERSIONFRAME_H