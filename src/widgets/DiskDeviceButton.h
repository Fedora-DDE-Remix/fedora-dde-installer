#ifndef DISK_DEVICEBUTTON_H
#define DISK_DEVICEBUTTON_H

#include "structs.h"
#include "DeviceSizeBar.h"

#include <QWidget>
#include <DTipLabel>

class QLabel;
DWIDGET_USE_NAMESPACE

class DiskDeviceButton : public QWidget {
    Q_OBJECT
public:
    explicit DiskDeviceButton(QWidget *parent = Q_NULLPTR);
    void setDeviceInfo(const SDeviceInfo &info);

signals:
    void clicked(const SDeviceInfo &info);

protected:
    bool eventFilter(QObject* watched, QEvent* event);

private:
    SDeviceInfo m_deviceInfo;
    QLabel *m_iconLabel = Q_NULLPTR;
    DTipLabel *m_deviceInfoLabel = Q_NULLPTR;
    QLabel *m_deviceNameLabel;
    bool m_isSelect = false;
};

#endif // DISK_DEVICEWIDGET_H
