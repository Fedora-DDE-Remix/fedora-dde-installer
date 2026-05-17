#include "SizeSliderWidget.h"
#include "CommonFunc.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QtMath>

RoundWidget::RoundWidget(QWidget *parent):QWidget (parent)
{
    setFixedSize(16, 16);
}

void RoundWidget::paintEvent(QPaintEvent *ev)
{
    QStyleOption opt;
    opt.initFrom(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(opt.palette.color(QPalette::Window)));
    painter.drawRoundedRect(ev->rect(), 4, 4);
}

SliderHandle::SliderHandle(QWidget *parent):QFrame (parent), m_isPress(false)
{
    setFixedWidth(6);
    setFrameStyle(QFrame::Panel | QFrame::Raised);
}

void SliderHandle::paintEvent(QPaintEvent *ev)
{
    QFrame::paintEvent(ev);
    QStyleOption option;
    option.initFrom(this);
    QRect bgRect = ev->rect();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush("#F0F0F0"));
    painter.drawRect(bgRect);
    //draw triangle
    int length = 6;
    QPoint mid = bgRect.topRight() + QPoint(0, bgRect.height() / 2);
    QPoint leftUp(bgRect.topLeft().x() + 2, mid.y() - length / 2);
    QPoint leftDown(leftUp.x(), mid.y() + length / 2);
    QPainterPath path;
    path.moveTo(leftUp);
    path.lineTo(mid);
    path.lineTo(leftDown);
    path.closeSubpath();

    bool bEnabled = (option.state & QStyle::State_Enabled) && (option.state & QStyle::State_Active);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QColor triangleColor = bEnabled ? QColor("#B2000000") : option.palette.color(QPalette::Inactive, QPalette::WindowText);
#else
    QColor triangleColor = bEnabled ? QColor("#B2000000") : option.palette.color(QPalette::Inactive, QPalette::WindowText);
#endif
    painter.fillPath(path, triangleColor);
}

void SliderHandle::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        m_isPress = true;
        m_lastPos = event->globalPosition().toPoint();
    }

    QWidget::mousePressEvent(event);
}

void SliderHandle::mouseMoveEvent(QMouseEvent *event)
{
    if(m_isPress) {
        Q_EMIT pressMove(event->globalPosition().toPoint().x()- m_lastPos.x());
        m_lastPos = event->globalPosition().toPoint();
    }
}

void SliderHandle::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && m_isPress) {
        m_isPress = false;
        m_lastPos = QPoint();
    }

    QWidget::mouseReleaseEvent(event);
}

SizeSliderWidget::SizeSliderWidget(QWidget *parent) : QWidget(parent), m_handle(new SliderHandle), m_sysLabel(new QLabel),
    m_dataLabel(new QLabel), m_otherLabel(new QLabel)
{
    m_sysLabel->setStyleSheet("background-color:#0081FF");
    m_dataLabel->setStyleSheet("background-color:#44D7B6");
    m_otherLabel->setStyleSheet("background-color:#D8D8D8");
    QHBoxLayout *mainlayout = new QHBoxLayout;
    mainlayout->setContentsMargins(0, 0, 0, 0);
    mainlayout->setSpacing(0);

    mainlayout->addWidget(m_sysLabel);
    mainlayout->addWidget(m_handle);
    mainlayout->addWidget(m_dataLabel);
    mainlayout->addWidget(m_otherLabel);
    setLayout(mainlayout);

    m_clip = new DClipEffectWidget(this);
    connect(m_handle, &SliderHandle::pressMove, this, &SizeSliderWidget::onHandleMove);
}

void SizeSliderWidget::initPartitionSize(int total, int sysMin, int other)
{
    m_totalSize = total;
    m_sysSize = sysMin;
    m_sysMinSize = CommonFunc::mToG(CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG"));
    m_otherSize = other;
    int dataMinSize = CommonFunc::mToG(CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG"));
    m_dataSize = m_totalSize - m_sysSize - m_otherSize;
    m_sysLabel->setFixedWidth(getPartitionWidth(m_sysSize));
    m_otherLabel->setFixedWidth(getPartitionWidth(m_otherSize));
    //防止整数产生的误差
    m_dataLabel->setFixedWidth(width() - m_sysLabel->width() - m_otherLabel->width() - m_handle->width());

    m_minXPos = getPartitionWidth(m_sysMinSize);
    m_sysMaxSize = m_totalSize - dataMinSize - m_otherSize;
    m_maxXPos = getPartitionWidth(m_sysMaxSize);

    Q_EMIT sizeChange(m_sysSize, m_dataSize, m_otherSize);
}

void SizeSliderWidget::setSysSize(int sys)
{
    if(sys < m_sysMinSize || sys > m_sysMaxSize) {
        return;
    }

    if(sys != m_sysSize) {
        m_sysSize = sys;
        m_dataSize = m_totalSize - m_otherSize - m_sysSize;
        updateHandlePos();
        Q_EMIT sizeChange(m_sysSize, m_dataSize, m_otherSize);
    }
}

void SizeSliderWidget::showEvent(QShowEvent *event)
{
    QPainterPath path;
    path.addRoundedRect(QRect(0, 0, width(), height()), 8, 8);
    m_clip->setClipPath(path);
    QWidget::showEvent(event);
}

void SizeSliderWidget::onHandleMove(int distance)
{
    int xpos = m_handle->x();
    int newX = xpos + distance;
    if(newX < m_minXPos || newX > m_maxXPos) {
        return;
    }

    int length = m_maxXPos - m_minXPos;
    int sysRange = m_sysMaxSize - m_sysMinSize;

    updateLabelWidth(distance);
    m_handle->move(newX, 0);

    m_sysSize = (m_sysLabel->width() - m_minXPos) * sysRange / length  + m_sysMinSize;
    m_dataSize = m_totalSize - m_sysSize - m_otherSize;
    Q_EMIT sizeChange(m_sysSize, m_dataSize, m_otherSize);

    update();
}

int SizeSliderWidget::getPartitionWidth(int partSize)
{
    int result = 0;
    int totalSize = width() - m_handle->width();
    result = totalSize * partSize / m_totalSize;
    return result;
}

void SizeSliderWidget::updateHandlePos()
{
    int length = m_maxXPos - m_minXPos;
    int sysRange = m_sysMaxSize - m_sysMinSize;
    int newX = (m_sysSize - m_sysMinSize) * length / sysRange + m_minXPos;
    int distance = newX - m_handle->x();

    updateLabelWidth(distance);
    m_handle->move(newX, 0);

    update();
}

int SizeSliderWidget::getPartitionSize(int partWidth)
{
    int totalWidth = width() - m_handle->width();
    return partWidth * m_totalSize / totalWidth;
}

void SizeSliderWidget::updateLabelWidth(int distance)
{
    int leftWidth = m_sysLabel->width();
    int rightWidth = m_dataLabel->width();
    leftWidth += distance;
    rightWidth -= distance;
    m_sysLabel->setFixedWidth(leftWidth);
    m_dataLabel->setFixedWidth(rightWidth);
}
