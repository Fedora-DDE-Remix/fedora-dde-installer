#ifndef SAVEDEVICEWIDGET_H
#define SAVEDEVICEWIDGET_H

#include <DDialog>
#include <QTimer>


class QListWidgetItem;
class QVBoxLayout;
class QHBoxLayout;
class QListWidget;
class QPushButton;
class TitleWidget;

DWIDGET_USE_NAMESPACE

class SaveDeviceWidget : public DDialog
{
    Q_OBJECT

public:
    explicit SaveDeviceWidget(QWidget *parent = nullptr);

    void saveList(const QStringList &fileList);
    void setTitleText(const QString &text, const QString &style);
    void setTipsText(const QString &text, const QString &strStyleSheet, bool isUseDefaultWidth = true);
    QString getCurrentSelectDevice();
    bool getSaveStatus();

private slots:
    void saveBtnClickedSlot();
    void cancelBtnClickedSlot();
    void deviceListItemClickedSlot();
    void updateDevicesList();

private:
    void initUI();
    QListWidgetItem* addInfoListItem(const QString &devicePath, const QString &mountPoint, qint64 devSize, qint64 devFreeSize);
     // 设置选中样式
    void setItemSelectStyle(QListWidgetItem *item, bool isSelect);
    void updateTitleText();
    void clearItemSelectStatus();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    TitleWidget *m_titleWidget = nullptr;
    QListWidget* m_deviceList = nullptr;
    QPushButton* m_saveBtn    = nullptr;
    QPushButton* m_cancelBtn  = nullptr;
    bool m_isExecSave = false;
    QTimer m_timer;
    QStringList m_fileList;
    QString m_currSelectDeviceName;

};

#endif // SAVEDEVICEWIDGET_H
