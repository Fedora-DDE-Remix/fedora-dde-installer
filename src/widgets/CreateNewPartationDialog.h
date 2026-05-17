#ifndef CREATENEWPARTATIONDIALOG_H
#define CREATENEWPARTATIONDIALOG_H

#include "structs.h"

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>
#include <DComboBox>

DWIDGET_USE_NAMESPACE

class TitleWidget;
class PartitionSizeSlider;
// todo 拼写错误
class CreateNewPartitionDialog : public DDialog
{
    Q_OBJECT
public:
    explicit CreateNewPartitionDialog(const SDeviceInfo &dev, const SPartitionInfo &free, QWidget *parent = Q_NULLPTR);
    void setPartitionTypes(const QMap<QString, EPartitionType> &types);
    void setPartitionLocation(const QList<std::pair<QString, bool> > &locations);
    SPartitionInfo getPartition();
    bool getFormatPartitionCheckStatus();

private:
    void initPartition();
    bool limitEditorSizeByFileSystem();
    bool limitEditorSizeByMountPoint();
    void updateErrTips(const QString &err);
    QStringList getFileSystemList();

    bool checkSliderRange(qint64 minSize, qint64 maxSize, bool bUseMin);
private:
    TitleWidget *m_titleWidget = Q_NULLPTR;
    DLabel *m_typeLabel = Q_NULLPTR;
    DComboBox *m_typeCmb = Q_NULLPTR;
    DLabel *m_locationLabel = Q_NULLPTR;
    DComboBox *m_locationCmb = Q_NULLPTR;
    DLabel *m_fileSystemLabel = Q_NULLPTR;
    DComboBox *m_fileSystemCmb = Q_NULLPTR;
    DLabel *m_mountPointLabel = Q_NULLPTR;
    DComboBox *m_mountPointCmb = Q_NULLPTR;
    DLabel *m_sizeLabel = Q_NULLPTR;
    DLabel *m_tipsLabel = Q_NULLPTR;
    PartitionSizeSlider *m_editSlider = Q_NULLPTR;
    QAbstractButton *m_cancelBtn = Q_NULLPTR;
    QAbstractButton *m_okBtn = Q_NULLPTR;
    DCheckBox *m_formatPartition = Q_NULLPTR;

    SPartitionInfo m_newPartitionInfo;
    SPartitionInfo m_freePartitionInfo;
};

#endif // CREATENEWPARTATIONDIALOG_H
