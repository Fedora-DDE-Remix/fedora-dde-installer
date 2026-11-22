#ifndef SHOWLOGWIDGET_H
#define SHOWLOGWIDGET_H

#include <QWidget>
#include <DSuggestButton>

class QPlainTextEdit;
class QPushButton;

DWIDGET_USE_NAMESPACE

class ShowLogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ShowLogWidget(QWidget *parent = nullptr);
    void showLog(const QString &content);
    void showLogFile(const QString &filepath);

signals:
    void saveLogSignal();
    void exitInstallSignal();

private slots:
    void saveLogBtnClickedSlot();
    void exitInstallBtnClickedSlot();

private:
    void initUI();
    void initConnect();
    void updateTs();

protected:
    void showEvent(QShowEvent *event) override;

private:
    QPlainTextEdit* m_installLog = nullptr;
    QPushButton* m_exitInstallBtn = nullptr;
    DSuggestButton* m_saveLogBtn = nullptr;
};

#endif // SHOWLOGWIDGET_H
