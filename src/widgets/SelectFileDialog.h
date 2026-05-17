#ifndef SELECTFILEDIALOG_H
#define SELECTFILEDIALOG_H

#include "structs.h"

#include <DDialog>
#include <DLabel>
#include <DListView>
#include <DSpinner>
#include <DWidget>
#include <QTimer>
#include <DPushButton>
#include <QStackedLayout>

DWIDGET_USE_NAMESPACE

class ScanUimgWidget : public DWidget
{
    Q_OBJECT
public:
    explicit ScanUimgWidget(QWidget *parent = nullptr);
    ~ScanUimgWidget() override;
    void onStart();
    void onCancel();
    void onTimeout();
    void updateTs();
    QList<SUimgFileInfo> getUimgFileList() const;
Q_SIGNALS:
    void cancel();
    void finish(bool );
private:
    DSpinner* m_spinner = nullptr;
    DPushButton *m_cancelBtn = nullptr;
    DLabel *m_label = nullptr;
    QTimer *m_timer = nullptr;
    QList<SUimgFileInfo> m_fileList;
    int m_count = 0;
};

class ScanResultWidget : public DWidget
{
    Q_OBJECT
public:
    explicit ScanResultWidget(QWidget *parent = nullptr);
    ~ScanResultWidget() override;
    void updateTs();
Q_SIGNALS:
    void cancel();
    void retry();
private:
    DLabel *m_failImg= nullptr;
    DLabel *m_failText = nullptr;
    DPushButton *m_cancelBtn = nullptr;
    DPushButton *m_retryBtn = nullptr;
};

class UImgSelectWidget : public  DWidget
{
    Q_OBJECT
public:
    explicit UImgSelectWidget(QWidget *parent = nullptr);
    ~UImgSelectWidget() override;
    void resetData(QList<SUimgFileInfo> &files);
    void updateTs();
Q_SIGNALS:
    void cancel();
    void uimgSelect(const QString &imgPath, const QString &imgDevice);
private:
    DListView *m_fileView = nullptr;
    QStandardItemModel *m_model = nullptr;
    DPushButton *m_cancel = nullptr;
    DPushButton *m_ok = nullptr;
};




class UImgFileSelectDialog : public DDialog
{
    Q_OBJECT
public:
    explicit UImgFileSelectDialog(QWidget *parent = nullptr);

    void updateTs();
    void showEvent(QShowEvent *event) override;
Q_SIGNALS:
    void uimgSelect(const QString &imgPath, const QString &imgDevice);

private:
    QList<SUimgFileInfo> m_fileList;
    DWidget *m_widget = nullptr;
    QStackedLayout *m_stackedLayout = nullptr;
    ScanUimgWidget *m_scanUimgWidget = nullptr;
    UImgSelectWidget *m_uimgSelectWidget = nullptr;
    ScanResultWidget *m_scanResultWidget = nullptr;
};

#endif // SELECTFILEDIALOG_H
