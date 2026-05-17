#ifndef SIZEEDITDIALOG_H
#define SIZEEDITDIALOG_H

#include <DDialog>
#include <DSlider>
#include <DLabel>
#include <DPushButton>
#include <DPasswordEdit>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE
class PartitionSizeSlider;
class TitleWidget;

class MultidiskSizeDialog:public DDialog
{
    Q_OBJECT
public:
    explicit MultidiskSizeDialog(QWidget *parent = nullptr);

    void initDiskSize(int total, int sysMin, int sys, int other);

    int getSysDiskSize();
private:
    void initConnection();
private:
    int m_total;
    int m_dataMin;
    int m_other;
    DSlider *m_sysSizeSlider;
    DLineEdit *m_sysSizeEdit;
    DLabel *m_labelDataSize;
    DLabel *m_labelOtherSize;
};

class SizeEditDialog : public DDialog
{
    Q_OBJECT
public:
    explicit SizeEditDialog(QWidget *parent = Q_NULLPTR);

    qint64 getSize();
    void setValueRange(qint64 minSize, qint64 totalSize, qint64 minDataSize);
    void setDefaultSize(qint64 nSize);
    void setDataSpaceSize(qint64 nSize);
    void updateTs();
    void reSetValues();
private:
    void initConnection();
    void updateBtnStatus();

signals:
    void sendValueChanged(qint64 nSize);

private:
    DLabel *m_SystemLabel = Q_NULLPTR;
    DLabel *m_DataLabel = Q_NULLPTR;
    DLabel *m_DataShowLabel = Q_NULLPTR;
    PartitionSizeSlider *m_editSlider = Q_NULLPTR;
    qint64 m_totalSize;
};

#endif // SIZEEDITDIALOG_H
