#include "MultipleDiskItem.h"
#include "CommonFunc.h"

#include <QStandardItem>
#include <QHBoxLayout>
#include <QToolTip>

#include <DGuiApplicationHelper>
#include <DFontSizeManager>

ClickWidget::ClickWidget(QWidget* parent) :
    DWidget (parent),
    m_item(Q_NULLPTR)
{

}

void ClickWidget::setItem(QStandardItem* item)
{
    m_item = item;
}

void ClickWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_item) {
        Q_EMIT signalItemClicked(m_item->index());
    }

    DWidget::mousePressEvent(event);
}

MultipleDiskItem::MultipleDiskItem(QWidget *parent)
    : ClickWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    DLabel *spaceLabel = new DLabel(this);
    spaceLabel->setFixedSize(20, 20);
    spaceLabel->setAlignment(Qt::AlignLeft);
    m_lbChecked = new DLabel(this);
    m_lbChecked->setAlignment(Qt::AlignRight);
    m_lbChecked->setFixedSize(20, 20);
    m_lbChecked->setPixmap(CommonFunc::loadSvgImage(":/images/select_white.svg", m_lbChecked->width(), m_lbChecked->height()));
    m_lbChecked->setFocusPolicy(Qt::NoFocus);
    QHBoxLayout* pHCheckedLayout = new QHBoxLayout;
    pHCheckedLayout->addWidget(spaceLabel);
    pHCheckedLayout->addStretch();
    pHCheckedLayout->addWidget(m_lbChecked);

    m_lbIcon = new DLabel(this);
    m_lbIcon->setFixedSize(48, 48);
    m_lbIcon->setPixmap(CommonFunc::loadSvgImage(":/images/drive-harddisk-128px.svg", m_lbIcon->width(), m_lbIcon->height()));
    m_lbIcon->setFocusPolicy(Qt::NoFocus);
    QHBoxLayout* pIconLayout = new QHBoxLayout;
    pIconLayout->setContentsMargins(0, 0, 0, 0);
    pIconLayout->addStretch();
    pIconLayout->addWidget(m_lbIcon);
    pIconLayout->addStretch();

    m_lbDeviceName = new DLabel(this);
    m_lbDeviceName->setAlignment(Qt::AlignCenter);
    m_lbDeviceName->installEventFilter(this);
    m_lbDeviceName->setMouseTracking(true);
    m_lbDeviceName->setFixedWidth(CommonFunc::isBigScreen() ? devItem::kDevNameWidth : devItem::kDevNameWidthSmall);
    m_pebak = m_lbDeviceName->palette();
    m_lbDeviceName->setFocusPolicy(Qt::NoFocus);
    DFontSizeManager::instance()->bind(m_lbDeviceName, DFontSizeManager::T6, QFont::DemiBold);
    QHBoxLayout* pDeviceNameLayout = new QHBoxLayout;
    pDeviceNameLayout->setContentsMargins(0, 0, 0, 0);
    pDeviceNameLayout->addStretch();
    pDeviceNameLayout->addWidget(m_lbDeviceName);
    pDeviceNameLayout->addStretch();

    m_lbDeviceSize = new DTipLabel("", this);
    m_lbDeviceSize->setAlignment(Qt::AlignCenter);
    m_lbDeviceSize->installEventFilter(this);
    m_lbDeviceSize->setMouseTracking(true);
    m_lbDeviceSize->setFixedWidth(CommonFunc::isBigScreen() ? devItem::kDevSizeWidth : devItem::kDevSizeWidthSmall);
    m_lbDeviceSize->setFocusPolicy(Qt::NoFocus);
    DFontSizeManager::instance()->bind(m_lbDeviceSize, DFontSizeManager::T6, QFont::Medium);
    QHBoxLayout* pDeviceSizeLayout = new QHBoxLayout;
    pDeviceSizeLayout->setContentsMargins(0, 0, 0, 0);
    pDeviceSizeLayout->addStretch();
    pDeviceSizeLayout->addWidget(m_lbDeviceSize);
    pDeviceSizeLayout->addStretch();

    m_cbDeviceType = new DComboBox(this);
    m_cbDeviceType->installEventFilter(this);
    m_cbDeviceType->setMouseTracking(true);
    connect(m_cbDeviceType, SIGNAL(activated(const QString &)), this, SLOT(curChanged(const QString &)));
    QHBoxLayout* pComboBoxLayout = new QHBoxLayout;
    m_cbDeviceType->setFocusPolicy(Qt::NoFocus);
    m_cbDeviceType->setFixedSize(
            CommonFunc::isBigScreen() ? QSize(devItem::kDeviceTypeWidth, devItem::kDeviceTypeHeight) : QSize(devItem::kDeviceTypeWidthSmall, devItem::kDeviceTypeHeightSmall));
    pComboBoxLayout->setContentsMargins(0, 0, 0, 0);
    pComboBoxLayout->addStretch();
    pComboBoxLayout->addWidget(m_cbDeviceType);
    pComboBoxLayout->addStretch();

    QVBoxLayout* pMainLayout = new QVBoxLayout;
    pMainLayout->setSpacing(3);
    pMainLayout->addSpacing(10);
    pMainLayout->addLayout(pHCheckedLayout);
    pMainLayout->addLayout(pIconLayout);
    pMainLayout->addSpacing(5);
    pMainLayout->addLayout(pDeviceNameLayout);
    pMainLayout->addSpacing(-3);
    pMainLayout->addLayout(pDeviceSizeLayout);
    pMainLayout->addStretch();
    pMainLayout->addSpacing(10);
    pMainLayout->addLayout(pComboBoxLayout);

    QHBoxLayout* pHMainLayout = new QHBoxLayout;
    pHMainLayout->setContentsMargins(0, 0, 0, 0);
    pHMainLayout->addStretch();
    pHMainLayout->addLayout(pMainLayout);
    pHMainLayout->addStretch();

    setLayout(pHMainLayout);
}

void MultipleDiskItem::resetStatus(MultipleDiskItem* pPreviousWnd, bool bFlag)
{
    DPalette pal = pPreviousWnd->m_lbDeviceName->palette();
    bFlag == DGuiApplicationHelper::LightType ? pal.setColor(DPalette::WindowText, palette().highlightedText().color()) :
                                                pal.setColor(DPalette::WindowText,  palette().text().color());
    pPreviousWnd->m_lbDeviceName->setForegroundRole(DPalette::WindowText);
    pPreviousWnd->m_lbDeviceName->setPalette(pal);

    pPreviousWnd->m_lbDeviceSize->setForegroundRole(DPalette::TextTips);
    pPreviousWnd->m_lbDeviceSize->setPalette(pal);

    pPreviousWnd->m_lbChecked->setForegroundRole(DPalette::WindowText);
    pPreviousWnd->m_lbChecked->setPalette(pal);
}

void MultipleDiskItem::setCheckedLabelVisiable(bool isVisiable)
{
    m_lbChecked->setVisible(isVisiable);
}

void MultipleDiskItem::setComboxVisiable(bool isVisiable)
{
    m_cbDeviceType->setVisible(isVisiable);
}

EDevType MultipleDiskItem::getItemDevType()
{
    return m_devtype;
}

void MultipleDiskItem::setItemDevType(const EDevType &devtype)
{
    m_devtype = devtype;
}

void MultipleDiskItem::setSelectTextColor(bool isSelect)
{
    DPalette pe = m_lbDeviceName->palette();
    if (isSelect) {
        pe.setColor(QPalette::WindowText, QColor("#FFFFFF"));
        m_lbDeviceName->setBackgroundRole(DPalette::Window);
        m_lbDeviceName->setPalette(pe);
        m_lbDeviceSize->setBackgroundRole(DPalette::Window);
        m_lbDeviceSize->setPalette(pe);
        m_lbDeviceSize->DLabel::setForegroundRole(QPalette::WindowText);
    } else {
        m_lbDeviceName->setPalette(m_pebak);
        m_lbDeviceSize->setPalette(m_pebak);
        m_lbDeviceSize->setForegroundRole(DPalette::TextTips);
    }
}

bool MultipleDiskItem::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
        case QEvent::MouseMove: {
                QFontMetrics fm(this->font());
                QPoint pt = QCursor::pos();
                if (watched == m_lbDeviceName) {
                    pt = m_lbDeviceName->mapToGlobal(QPoint(0,0));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                    if (fm.horizontalAdvance(m_deviceInfo.name) > (CommonFunc::isBigScreen() ? devItem::kDevNameWidth : devItem::kDevNameWidthSmall)) {
#else
                    if (fm.width(m_deviceInfo.name) > (CommonFunc::isBigScreen() ? devItem::kDevNameWidth : devItem::kDevNameWidthSmall)) {
#endif
                        QToolTip::showText(QPoint(pt.x() + m_lbDeviceName->width() + 10, pt.y() - m_lbDeviceName->height() / 2),
                                           m_deviceInfo.name, this, this->rect(), devItem::kTooltipTime);
                        return true;
                    }
                }

                if (watched == m_lbDeviceSize) {
                    double freesize = m_deviceInfo.freesize;
                    double size = m_deviceInfo.size;
                    QString unit = CommonFunc::showSizeUnit(freesize, size);
                    QString strSize = QString("%1%2").arg(QString::number(size), unit);
                    pt = m_lbDeviceSize->mapToGlobal(QPoint(0,0));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                    if (fm.horizontalAdvance(strSize) > (CommonFunc::isBigScreen() ? devItem::kDevSizeWidth : devItem::kDevSizeWidthSmall)) {
#else
                    if (fm.width(strSize) > (CommonFunc::isBigScreen() ? devItem::kDevSizeWidth : devItem::kDevSizeWidthSmall)) {
#endif
                        QToolTip::showText(QPoint(pt.x() + m_lbDeviceSize->width() + 10, pt.y() - m_lbDeviceSize->height() / 2),
                                           strSize, this, this->rect(), devItem::kTooltipTime);
                        return true;
                    }
                }

                if (watched == m_cbDeviceType) {
                    pt = m_cbDeviceType->mapToGlobal(QPoint(0,0));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                    if (fm.horizontalAdvance(m_cbDeviceType->currentText()) > ((CommonFunc::isBigScreen() ? devItem::kDeviceTypeWidth : devItem::kDeviceTypeWidthSmall) - 42)) {
#else
                    if (fm.width(m_cbDeviceType->currentText()) > ((CommonFunc::isBigScreen() ? devItem::kDeviceTypeWidth : devItem::kDeviceTypeWidthSmall) - 42)) {
#endif
                        QToolTip::showText(QPoint(pt.x() + m_cbDeviceType->width() + 10, pt.y() - m_cbDeviceType->height() / 2),
                                           m_cbDeviceType->currentText(), this, this->rect(), devItem::kTooltipTime);
                        return true;
                    }
                }
            }
        break;
        default: break;
    }

    return QWidget::eventFilter(watched, event);
}

void MultipleDiskItem::setDeviceInfo(const SDeviceInfo &info)
{
    double freesize = info.freesize;
    double size = info.size;
    QString unit = CommonFunc::showSizeUnit(freesize, size);

    QFontMetrics fm(this->font());
    QString strDeviceName = fm.elidedText(info.name, Qt::ElideRight,
                                          CommonFunc::isBigScreen() ? devItem::kDevNameWidth : devItem::kDevNameWidthSmall);
    m_lbDeviceName->setText(strDeviceName);
    QString strDeviceSize = fm.elidedText(QString("%1%2").arg(QString::number(size), unit), Qt::ElideRight,
                                          CommonFunc::isBigScreen() ? devItem::kDevSizeWidth : devItem::kDevSizeWidthSmall);
    m_lbDeviceSize->setText(strDeviceSize);
    m_deviceInfo = info;
}

void MultipleDiskItem::setComboxActions(const QStringList &items)
{
    m_cbDeviceType->clear();
    if (items.size() > 0) {
        m_cbDeviceType->addItems(items);
    }
}

void MultipleDiskItem::setComboxCurText(const QString &text)
{
    m_cbDeviceType->setCurrentText(text);
    m_cbText = text;
}

QString MultipleDiskItem::getComboxCurText()
{
    return m_cbDeviceType->currentText();
}

void MultipleDiskItem::curChanged(const QString &text)
{
    if (!m_cbText.isEmpty() && text != m_cbText) {
        Q_EMIT comboBoxSelected(text);
    };
    m_cbText = text;
}


