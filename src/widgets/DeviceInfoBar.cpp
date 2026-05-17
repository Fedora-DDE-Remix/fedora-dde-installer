#include "DeviceInfoBar.h"
#include "CommonFunc.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <DFontSizeManager>

DeviceInfoBar::DeviceInfoBar(QWidget *parent)
    : DLabel(parent)
    , m_nSystemSize(-1)
{
    m_mainLayout = new QVBoxLayout;
    this->setLayout(m_mainLayout);
}

void DeviceInfoBar::addDeviceInfo(const SDeviceSizeInfo &deviceSizeInfo)
{
    if (m_deviceInfo.indexOf(deviceSizeInfo) != -1) {
        m_deviceInfo.removeAll(deviceSizeInfo);
    }

    m_deviceInfo.append(deviceSizeInfo);
    this->updateInfo();
}

void DeviceInfoBar::updateInfo()
{
    // 清理布局
    if (this->layout()) {
        CommonFunc::deleteLayout(this->layout());
    }

    m_mainLayout->setSpacing(8);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    // 添加子控件
    for (QList<SDeviceSizeInfo>::iterator it = m_deviceInfo.begin(); it != m_deviceInfo.end(); ++it) {
        QLabel *colorLabel = new QLabel(this);
        colorLabel->setFixedSize(10, 10);
        colorLabel->setStyleSheet(QString("QLabel{background-color:%1;}").arg(it->color));
        DLabel *nameLabel = new DLabel(this);
        nameLabel->setAlignment(Qt::AlignLeading | Qt::AlignVCenter);
        DFontSizeManager::instance()->bind(nameLabel, DFontSizeManager::T8);

        m_textLabel = new DLabel(this);
        m_textLabel->setFixedWidth(70);
        m_textLabel->setText(it->text);
        m_textLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QHBoxLayout *itemLayout = new QHBoxLayout;
        itemLayout->setContentsMargins(0, 0, 0, 0);
        itemLayout->setSpacing(10);
        itemLayout->addWidget(colorLabel, 0, Qt::AlignVCenter);
        itemLayout->addWidget(nameLabel, 0, Qt::AlignVCenter);
        itemLayout->addWidget(m_textLabel, 0, Qt::AlignVCenter);
        if (it->edit) {
            nameLabel->setText(::QObject::tr("System Space"));
            QString strSize = it->text;
            m_nSystemSize = strSize.replace(QRegularExpression("[\u4e00-\u9fa5_a-zA-Z]"), "").trimmed().toInt();
            m_editLabel = new DLabel(this);
            m_editLabel->installEventFilter(this);
            m_editLabel->setPixmap(CommonFunc::loadSvgImage(":/images/edit.svg", 16, 16));
            itemLayout->addWidget(m_editLabel, 0, Qt::AlignVCenter);
        } else {
            nameLabel->setText(::QObject::tr("Data space"));
            if (m_editLabel) {
                DLabel *spaceLabel = new DLabel(this);
                itemLayout->addWidget(spaceLabel);
            }
        }
        itemLayout->addStretch();

        m_mainLayout->addLayout(itemLayout);
    }

    this->setLayout(m_mainLayout);
}

bool DeviceInfoBar::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_editLabel) {
        switch (event->type()) {
            case QEvent::MouseButtonPress: Q_EMIT clicked(); break;
            case QEvent::Enter: setCursor(QCursor(Qt::PointingHandCursor)); break;
            case QEvent::Leave: setCursor(QCursor(Qt::ArrowCursor)); break;
            default: break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void DeviceInfoBar::setEnable(bool isEnable)
{
    isEnable ? m_editLabel->installEventFilter(this) : m_editLabel->removeEventFilter(this);
    this->setEnabled(isEnable);
}


