#ifndef SELECTFILEWIDGET_H
#define SELECTFILEWIDGET_H

#include "structs.h"

#include <DLabel>
#include <DTipLabel>
#include <DFrame>
#include <DLineEdit>
#include <DIconButton>

class UImgFileSelectDialog;

DWIDGET_USE_NAMESPACE

class SelectFileWidget : public DFrame
{
    Q_OBJECT

public:
    explicit SelectFileWidget(QWidget *parent = nullptr);

    QString getSelectFilePath();

    QString getSelectDevice();

    void updateTs();
Q_SIGNALS:
    void textChanged(const QString &);

protected:
    bool eventFilter(QObject* watched, QEvent* event);
private:
    DTipLabel *m_pathLabel;
    DIconButton *m_btnOpen;

    QString m_filePath;
    QString m_fileDevice;
    QString m_fileFilter;

    UImgFileSelectDialog *m_uimgDlg = nullptr;
};

#endif // SELECTFILEWIDGET_H
