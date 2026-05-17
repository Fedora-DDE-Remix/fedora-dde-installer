#ifndef PARTITIONENCRYPTDIALOG_H
#define PARTITIONENCRYPTDIALOG_H
#include <DDialog>
#include <DPasswordEdit>
#include <QCheckBox>
#include "HelpDialog.h"
#include "structs.h"
DWIDGET_USE_NAMESPACE

class PartitionDecryptDialog:public DDialog
{
    Q_OBJECT
public:
    explicit PartitionDecryptDialog(const QList<SPartitionInfo> &partInfo, QWidget *parent = nullptr);
protected:
    void showEvent(QShowEvent *event) override;
private:
    void updateMessageLabel();
private slots:
    void onHandleDecrypt();
private:
    QString m_message;
    DPasswordEdit *m_pwdEdit;
    DLabel *m_messageLabel;
    QList<SPartitionInfo> m_decrypList;
};

#endif // PARTITIONENCRYPTDIALOG_H
