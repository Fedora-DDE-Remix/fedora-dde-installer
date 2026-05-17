#ifndef EXPORTKEYDIALOG_H
#define EXPORTKEYDIALOG_H
#include "DDialog"
#include "DListView"
#include <QStandardItemModel>
#include <DProgressBar>
#include <DLabel>
#include <DTipLabel>
DWIDGET_USE_NAMESPACE

class UDiskDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit UDiskDelegate(QObject *parent = Q_NULLPTR);
protected:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};


class ExportFileDialog:public DDialog
{
    Q_OBJECT
public:
    explicit ExportFileDialog(QWidget *parent = nullptr);

    void setTitleAndMessage(const QString &title, const QString &message);

    void onDelUsbDevice(const QString &id);

    void setRecoveryKey(const QString &cryptKey);

    QString getRecoveryKey() {
        return m_cryptKey;
    }

    void getSelectedDevice(QString &device, QString &mountPoint) {
        device = m_devSave;
        mountPoint = m_devMount;
    }

    void updateTs();
private:
    void initUI();

    void initConnection();
protected:
    void showEvent(QShowEvent *event) override;
public slots:
    void onAddUsbDevice(const QString &id, const QString &diskName, const QString &mount, int total, int avail);
private:
    QListView *m_devList;
    QStandardItemModel *m_devModel;
    QAbstractButton *m_btnCancel;
    QAbstractButton *m_btnConfirm;
    DTipLabel *m_tipLabel;
    DLabel *m_labelKey;
    QString m_cryptKey;
    QString m_devSave;
    QString m_devMount;
};

class SaveFileToUsb: public QObject
{
    Q_OBJECT
public:
    explicit SaveFileToUsb(QObject *parent = nullptr);

    void setTitleAndMessage(const QString &title, const QString &msg)
    {
        m_title = title;
        m_message = msg;
    }

    void setRecoveryKey(const QString &key);

    void setSaveFileList(const  QStringList &fileList);

    void updateTs();
Q_SIGNALS:
    void copyDone(bool bSuccess, const QString &devId);
public slots:
    void beginSaveFile();
private slots:
    void onScanOver();
private:
    void exportFileToUsb();

    bool copyFiles(const QString &dest);
private:
    QStringList m_fileList;
    QString m_title;
    QString m_message;
    ExportFileDialog *m_epFileDlg;
    QTimer *m_timer;
    QStringList m_devList;
};

#endif // EXPORTKEYDIALOG_H
