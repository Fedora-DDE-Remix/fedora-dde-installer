#ifndef WARNNING_WIDGET_H
#define WARNNING_WIDGET_H

#include <DDialog>
#include <QEvent>

class QPushButton;
class QLabel;

DWIDGET_USE_NAMESPACE

class WarningDialog : public DDialog
{
    Q_OBJECT

public:
    explicit WarningDialog(QWidget* parent = nullptr);
    void setCommentText(const QString &text);
    void setBtnText(const QString &text);
    void setDialogIcon(const QIcon &icon);

signals:
    void enterBtnSignal();
    void closeBtnSignal();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void initConnections();
    void initUI();

private:
    QPushButton *m_enterButton = nullptr;
    QLabel *m_commentLabel = nullptr;
};

#endif // WARNNING_WIDGET_H
