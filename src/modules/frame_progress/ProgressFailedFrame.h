#ifndef INSTALLRESULTFRAME_H
#define INSTALLRESULTFRAME_H

#include <QWidget>

class QLabel;
class ShowLogWidget;
class SaveDeviceWidget;
class TitleWidget;
class QStackedWidget;
class SaveFileToUsb;

class ProgressFailedFrame : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressFailedFrame(QWidget *parent = nullptr);
    void showLog(const QString &content);
    void showLogFile(const QString &filepath);

private slots:
    void showSaveLogSlot();
    void installFailedSlot();

private:
    void initUI();
    void initConnect();
    void updateTs();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    TitleWidget *m_titleWidget = nullptr;
    ShowLogWidget *m_showLogWidget = nullptr;
    SaveDeviceWidget *m_saveLogWidget = nullptr;
    SaveFileToUsb *m_saveToUsb = nullptr;
};

#endif // INSTALLRESULTFRAME_H
