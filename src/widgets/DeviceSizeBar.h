#ifndef DEVICESIZEBAR_H
#define DEVICESIZEBAR_H

#include "structs.h"

#include <QWidget>
#include <QObject>
class DeviceSizeBar : public QWidget{
    Q_OBJECT
public:
    explicit DeviceSizeBar(QWidget *parent = Q_NULLPTR);
    void addDeviceSize(const SDeviceSizeInfo &deviceSizeInfo);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QList<SDeviceSizeInfo> m_deviceInfo;// 显示的方块的颜色和空间的大小
};

#endif // DEVICESIZEBAR_H
