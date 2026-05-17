#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <DBlurEffectWidget>
#include <DLabel>
#include <QVBoxLayout>
#include <DDialog>

DWIDGET_USE_NAMESPACE

class HelpDialog: public DBlurEffectWidget
{
    Q_OBJECT
public:
    explicit HelpDialog(QWidget *parent = Q_NULLPTR);

    explicit HelpDialog(int width, QWidget *parent = Q_NULLPTR);

    virtual void setContentText();

    void updateTs();

    void addHelpText(const QString &title, const QString &description);
public slots:
    void showCenter(QWidget *showWidget);
protected:
    void initUI();
protected:
    QVBoxLayout *mainLayout;
    int m_dlgWidth;
};

class SeniorTipDialog: public HelpDialog
{
    Q_OBJECT
public:
    explicit SeniorTipDialog(QWidget *parent = nullptr);

    void setContentText() override;
};

class BackupTipDialog: public HelpDialog
{
    Q_OBJECT
public:
    explicit BackupTipDialog(QWidget *parent = nullptr);

    void setContentText() override;
};

class AutoDecryptDialog: public HelpDialog
{
    Q_OBJECT
public:
    explicit AutoDecryptDialog(QWidget *parent = nullptr);

    void setContentText() override;
};

class OtherPartitionDialog : public  HelpDialog
{
    Q_OBJECT
public:
    explicit OtherPartitionDialog(QWidget *parent = nullptr);

    void setContentText() override;
};
#endif // HELPDIALOG_H
