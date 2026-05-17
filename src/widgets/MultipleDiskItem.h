#ifndef MULTIPLEDISKITEM_H
#define MULTIPLEDISKITEM_H

#include "structs.h"

#include <DWidget>
#include <DComboBox>
#include <DLabel>
#include <DTipLabel>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE
DWIDGET_BEGIN_NAMESPACE
DWIDGET_END_NAMESPACE

namespace devItem {
    const int kDevNameWidth = 120;
    const int kDevNameWidthSmall = 100;
    const int kDevSizeWidth = 120;
    const int kDevSizeWidthSmall = 100;

    const int kDeviceTypeWidth = 120;
    const int kDeviceTypeHeight = 36;
    const int kDeviceTypeWidthSmall = 100;
    const int kDeviceTypeHeightSmall = 36;

    const int kTooltipTime = 1000;
}

class QStandardItem;
class ClickWidget : public DWidget
{
    Q_OBJECT

public:
    explicit ClickWidget(QWidget* parent = Q_NULLPTR);

    void setItem(QStandardItem* item);

protected:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

signals:
    void signalItemClicked(const QModelIndex &);

private:
    QStandardItem*  m_item;

};

class MultipleDiskItem : public ClickWidget
{
    Q_OBJECT

public:
    explicit MultipleDiskItem(QWidget* pParent = Q_NULLPTR);
    void setDeviceInfo(const SDeviceInfo &info);
    void resetStatus(MultipleDiskItem* pPreviousWnd, bool bFlag);
    void setCheckedLabelVisiable(bool isVisiable);
    void setComboxActions(const QStringList &items);
    void setComboxCurText(const QString &text);
    QString getComboxCurText();
    void setComboxVisiable(bool isVisiable);
    EDevType getItemDevType();
    void setItemDevType(const EDevType &devtype);
    void setSelectTextColor(bool isSelect);

signals:
   void comboBoxSelected(const QString &);

public slots:
   void curChanged(const QString &text);

protected:
   virtual bool eventFilter(QObject* watched, QEvent* event) Q_DECL_OVERRIDE;

private:
    SDeviceInfo m_deviceInfo;

    DLabel *m_lbIcon = Q_NULLPTR;
    DLabel *m_lbChecked = Q_NULLPTR;
    DLabel *m_lbDeviceName = Q_NULLPTR;
    DTipLabel *m_lbDeviceSize = Q_NULLPTR;
    DComboBox *m_cbDeviceType = Q_NULLPTR;
    EDevType m_devtype;

    DPalette m_pebak = DPalette();

    QString m_cbText = Q_NULLPTR;
};

#endif // MULTIPLEDISKITEM_H
