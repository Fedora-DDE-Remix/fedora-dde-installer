#include "SystemInfoTip.h"
#include "CommonFunc.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QDebug>

#include <DApplication>
#include <DGuiApplicationHelper>

const int kWindowTopMargin = -3;

const int kBorderRadius = 8;
const int kBorderDiameter = kBorderRadius * 2;

// tip下面的小三角的尺寸
const int kTriangleHeight = 6;
const int kTriangleWidth = 10;
const int kTriangleLeftMargin = 36;

const int kMaximumWindowWidth = 310;
const int kMinimumWindowHeight = 32;
const int kWindowMarginLeft = 8;
const int kWindowMarginTop = 10;
const int kWindowMarginRight = 8;
const int kWindowMarginBottom = 6;
const int kMaximumLabelWidth = kMaximumWindowWidth - kWindowMarginLeft -
                               kWindowMarginRight;

SystemInfoTip::SystemInfoTip(QWidget *parent)
    : QFrame (parent)
{
    m_label = new QLabel(this);
    m_label->setWordWrap(true);
    m_label->setMaximumWidth(kMaximumLabelWidth);

    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(kWindowMarginLeft, kWindowMarginTop, kWindowMarginRight, kWindowMarginBottom);
    layout->addWidget(m_label);

    setLayout(layout);
    setMaximumWidth(kMaximumWindowWidth);
    setMinimumHeight(kMinimumWindowHeight);
}

void SystemInfoTip::setText(const QString &text)
{
    m_label->setText(text);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    const int width = QFontMetrics(m_label->font()).horizontalAdvance(text);
#else
    const int width = QFontMetrics(m_label->font()).width(text);
#endif

    if (width < kMaximumLabelWidth) {
        m_label->setWordWrap(false);
        m_label->setFixedWidth(width);
    } else {
        m_label->setWordWrap(true);
        m_label->setFixedWidth(kMaximumLabelWidth);
    }

    this->adjustSize();
}

void SystemInfoTip::setTextColor(const QString &color)
{
    m_label->setStyleSheet(QString("color:%1").arg(color));
}

void SystemInfoTip::showBottom(const QWidget *widget)
{
    if (widget == nullptr) {
        qWarning() << "The prompt widget is nullptr";
        return;
    }

    QPoint point = widget->mapToGlobal(QPoint(0, 0)) + QPoint(0, widget->height() + kWindowTopMargin);

    if (parentWidget() == nullptr) {
        move(point);
    } else {
        move(parentWidget()->mapFromGlobal(point));
    }

    show();
    raise();
}

void SystemInfoTip::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, true);
    const int wd = this->width();
    const int hgt = this->height();

    // 绘制圆角背景
    QPainterPath path;
    path.moveTo(wd, kBorderRadius + kTriangleHeight);
    path.arcTo(wd - kBorderDiameter, kTriangleHeight,
               kBorderDiameter, kBorderDiameter,
               0.0, 90.0);

    // 绘制等腰直角三角形
    path.lineTo(kTriangleLeftMargin + kTriangleWidth, kTriangleHeight);
    path.lineTo(kTriangleLeftMargin + kTriangleHeight, 0);
    path.lineTo(kTriangleLeftMargin, kTriangleHeight);
    path.lineTo(kBorderRadius, kTriangleHeight);
    path.arcTo(0, kTriangleHeight,
               kBorderDiameter, kBorderDiameter,
               90.0, 90.0);

    path.lineTo(0, hgt - kBorderRadius);
    path.arcTo(0, hgt - kBorderDiameter,
               kBorderDiameter, kBorderDiameter,
               180.0, 90.0);

    path.lineTo(wd - kBorderRadius, hgt);
    path.arcTo(wd - kBorderDiameter, hgt - kBorderDiameter,
               kBorderDiameter, kBorderDiameter,
               270.0, 90.0);

    path.closeSubpath();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QBrush bgBrush(Dtk::Gui::DPalette().color(Dtk::Gui::DPalette::Active, Dtk::Gui::DPalette::Window));
#else
    QBrush bgBrush(DPalette().color(DPalette::Active, DPalette::Window));
#endif
    painter.fillPath(path, bgBrush);
}
