#ifndef INSTALLSUCCESSWIDGET_H
#define INSTALLSUCCESSWIDGET_H

#include <QWidget>
#include <DSuggestButton>
#include <DCommandLinkButton>

class QLabel;
class QPushButton;
class SaveDeviceWidget;
class SaveFileToUsb;

DWIDGET_USE_NAMESPACE

class ProgressSuccessFrame : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressSuccessFrame(QWidget *parent = nullptr);

private slots:
    void installOkSlot();
    void exportKeySlot();
private:
    void initUI();
    void updateTs();

    void initConnection();
protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QLabel* m_titleLabel = nullptr;
    QWidget *m_appDetail;
    QLabel *m_descLabel = nullptr;
    DCommandLinkButton *m_btnDetail;
    QLabel *m_infoTipLabel = nullptr;
    QPushButton *m_exportKey = nullptr;
    DSuggestButton *m_exitInstallBtn = nullptr;
    SaveFileToUsb *m_saveToUsb = nullptr;
};

#endif // INSTALLSUCCESSWIDGET_H
