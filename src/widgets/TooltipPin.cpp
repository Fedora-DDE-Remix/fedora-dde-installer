#include "TooltipPin.h"

#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

const int kBorderRadius = 8;
const int kBorderDiameter = kBorderRadius * 2;

// 小三角的高度
const int kTriangleHeight = 6;

const int kTextBaseLine = 17;

TooltipPin::TooltipPin(QWidget *parent)
    : QLabel (parent)
    , m_ot(Orientation::Down)
{
    setAlignment(Qt::AlignHCenter);

    // 上下不留，左右留间隙
    setStyleSheet("margin: 0 15px;");
}

void TooltipPin::setOrientation(const Orientation ot)
{
    m_ot = ot;

    update();
}

void TooltipPin::popup(const QPoint& point)
{
    move(point);
    show();
}

void TooltipPin::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing,
                           true);
    const int wd = width();
    const int halfWd = wd / 2;
    const int hgt = height();

    // 画圆角矩形部分
    QPainterPath rectPath;
    rectPath.moveTo(wd, kBorderRadius);
    rectPath.arcTo(wd - kBorderDiameter, 0,
                          kBorderDiameter, kBorderDiameter,
                          0.0, 90.0);

    int leftX = 0;
    if (m_ot == Orientation::Left) {
        leftX += kTriangleHeight;
    }
    rectPath.lineTo(leftX + kBorderRadius, 0);
    rectPath.arcTo(leftX, 0, kBorderDiameter, kBorderDiameter, 90.0, 90.0);
    rectPath.lineTo(leftX, hgt - kBorderRadius - kTriangleHeight);
    rectPath.arcTo(leftX, hgt - kBorderDiameter - kTriangleHeight,
                          kBorderDiameter, kBorderDiameter,
                          180.0, 90.0);
    rectPath.lineTo(wd - kBorderRadius, hgt - kTriangleHeight);
    rectPath.arcTo(wd - kBorderDiameter,
                          hgt - kBorderDiameter - kTriangleHeight,
                          kBorderDiameter, kBorderDiameter,
                          270.0, 90.0);
    rectPath.lineTo(wd, kBorderRadius);
    rectPath.closeSubpath();

    // 进行平移变换
    int textBaseLine = kTextBaseLine;
    if (m_ot == Orientation::Up) {
        rectPath.translate(0, kTriangleHeight);
        textBaseLine += kTriangleHeight;
    }

    // 画三角部分
    QPainterPath trianglePath;
    QList<QPoint> trianglePoints;

    trianglePoints << QPoint(halfWd - kTriangleHeight, hgt - kTriangleHeight)
                   << QPoint(halfWd, hgt)
                   << QPoint(halfWd + kTriangleHeight, hgt - kTriangleHeight)
                   << QPoint(halfWd - kTriangleHeight, hgt - kTriangleHeight);

    if (m_ot == Orientation::Left) {
        trianglePoints.clear();
        trianglePoints << QPoint(kTriangleHeight, (hgt - kTriangleHeight) / 2 - kTriangleHeight)
                       << QPoint(0, (hgt - kTriangleHeight) / 2)
                       << QPoint(kTriangleHeight, (hgt - kTriangleHeight) / 2 + kTriangleHeight)
                       << QPoint(kTriangleHeight, (hgt - kTriangleHeight) / 2 - kTriangleHeight);
    } else if (m_ot == Orientation::Up) {
        trianglePoints.clear();
        trianglePoints << QPoint(halfWd - kTriangleHeight, kTriangleHeight)
                       << QPoint(halfWd, 0)
                       << QPoint(halfWd + kTriangleHeight, kTriangleHeight)
                       << QPoint(halfWd - kTriangleHeight, kTriangleHeight);
    }

    for (auto& it : trianglePoints) {
        trianglePath.lineTo(it);
    }
    trianglePath.closeSubpath();

    // 合并两部分
    rectPath.addPath(trianglePath);

    painter.fillPath(rectPath, QBrush(Qt::white));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    const int length = QFontMetrics(font()).horizontalAdvance(text());
#else
    const int length = QFontMetrics(font()).width(text());
#endif

    painter.setPen(QPen(Qt::black));
    painter.setFont(font());

    painter.drawText((wd - length) / 2 + 2, textBaseLine, text());
}
