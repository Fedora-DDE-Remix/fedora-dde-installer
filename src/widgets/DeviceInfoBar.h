#ifndef DEVICEINFOBAR_H
#define DEVICEINFOBAR_H

#include "structs.h"

#include <DLabel>

DWIDGET_USE_NAMESPACE

class QVBoxLayout;
class DeviceSizeBar;
class DeviceInfoBar : public DLabel {
    Q_OBJECT
public:
    explicit DeviceInfoBar(QWidget *parent = Q_NULLPTR);
    void addDeviceInfo(const SDeviceSizeInfo &deviceSizeInfo);
    void setEnable(bool isEnable);

protected:
    bool eventFilter(QObject* watched, QEvent* event);

signals:
    void clicked();

private:
    void updateInfo();

private:
    QList<SDeviceSizeInfo> m_deviceInfo;   // 显示的方块的颜色和显示的信息
    QVBoxLayout *m_mainLayout = Q_NULLPTR;
    DLabel *m_editLabel = Q_NULLPTR;
    DLabel *m_textLabel = Q_NULLPTR;
    qint64 m_nSystemSize = -1;
};

#endif // DEVICEINFOBAR_H
