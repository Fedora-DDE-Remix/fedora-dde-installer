#ifndef OPTIONBUTTON_H
#define OPTIONBUTTON_H

#include <DPushButton>
#include <DTipLabel>

DWIDGET_USE_NAMESPACE

class QLabel;
class OptionButton : public DPushButton
{
    Q_OBJECT
public:
    explicit OptionButton(const int iconOffSet, QWidget *parent = Q_NULLPTR);
    void setSelect(const OptionButton &pOptionButton);
    void setInstallTypeText(const QString &strInstallTypeText = Q_NULLPTR, const QString &strStyleSheet = Q_NULLPTR);
    void setIntroductionText(const QString &strIntroductionText = Q_NULLPTR, const QString &strStyleSheet = Q_NULLPTR);
    void setIcon(const QPixmap &icon = QPixmap());
    void setNavIcon(const QPixmap &icon = QPixmap());
    void setIntroductionVisible(bool isVisibel = true);

signals:
    void select(const OptionButton &pOptionButton);

protected:
    virtual void enterEvent(QEnterEvent *e) Q_DECL_OVERRIDE;//鼠标进入事件
    virtual void leaveEvent(QEvent *e) Q_DECL_OVERRIDE;//鼠标离开事件

private:
    QLabel *m_iconLabel = Q_NULLPTR;
    QLabel *m_installTypeLabel = Q_NULLPTR;
    DTipLabel *m_introductionLabel = Q_NULLPTR;
    QLabel *m_navLabel = Q_NULLPTR;
    QWidget *m_contentWidget = Q_NULLPTR;

};

#endif // OPTIONWIDGET_H
