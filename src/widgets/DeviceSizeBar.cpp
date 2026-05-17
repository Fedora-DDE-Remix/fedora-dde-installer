#include "DeviceSizeBar.h"

#include <QColor>
#include <QPainter>
#include <QHash>
#include <QPen>
#include <QPainterPath>
#include <QDebug>

DeviceSizeBar::DeviceSizeBar(QWidget *parent):
    QWidget (parent)
{

}

void DeviceSizeBar::addDeviceSize(const SDeviceSizeInfo &deviceSizeInfo)
{
    if (m_deviceInfo.indexOf(deviceSizeInfo) != -1) {
        m_deviceInfo.removeAll(deviceSizeInfo);
    }

    m_deviceInfo.append(deviceSizeInfo);
    this->update();
}

void DeviceSizeBar::paintEvent(QPaintEvent *event)
{
    //绘制准备工作,启用反锯齿,平移坐标轴中心,等比例缩放
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.save();

    int sum = 0;
    for (auto info : m_deviceInfo) {
        sum += info.size;
    }

    int pos = 0;
    for (QList<SDeviceSizeInfo>::iterator it = m_deviceInfo.begin(); it != m_deviceInfo.end(); ++it) {
        double percent = (double)it->size / sum;
        int width = this->width() * percent;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(it->color));

        //计算绘制的区域,需要裁剪圆角部分
        QPainterPath clipPath;
        clipPath.addRoundedRect(this->rect(), this->height() / 2, this->height() / 2);
        painter.setClipPath(clipPath);

        QRect rect(pos, 0, width + 4, this->height());
        painter.drawRect(rect);
        pos += width;
    }

    painter.restore();
}
