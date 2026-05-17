#ifndef TITLEWIDGETS_H
#define TITLEWIDGETS_H

#include <QWidget>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class QLabel;
class TitleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TitleWidget(QWidget *parent = nullptr);

    void setTitleLabelProperty(const QString &strTitle  = Q_NULLPTR, const QString &strStyleSheet = Q_NULLPTR);
    void setTipsLabelProperty(const QString &strTips = Q_NULLPTR, const QString &strStyleSheet = Q_NULLPTR, bool isUseDefaultWidth = true);
    void setTipsLabelVisible(bool isVisible);
    void setTipsLabelHide();
    void setTipsLabelWidth(int width);
    int getTotalHeight();

private:
    QLabel *m_titleLabel = Q_NULLPTR;
    QLabel *m_tipsLabel = Q_NULLPTR;

};

#endif // TITLEWIDGETS_H
