#ifndef TOOLTIPPIN_H
#define TOOLTIPPIN_H

#include <QLabel>

enum Orientation{
    Up,
    Down,
    Left,
    Right
};

// 文本提示小工具，小工具的底部中间有一个小三角
class TooltipPin : public QLabel
{
    Q_OBJECT

public:
    explicit TooltipPin(QWidget* parent = nullptr);
    void setOrientation(const Orientation ot);

public slots:
    // 设置小工具的显示位置
    void popup(const QPoint& point);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Orientation m_ot = Orientation::Down;
};

#endif // TOOLTIPPIN_H
