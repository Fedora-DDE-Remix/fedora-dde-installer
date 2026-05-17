

#ifndef DEEPIN_INSTALLER_HELPRADIOBUTTON_H
#define DEEPIN_INSTALLER_HELPRADIOBUTTON_H

#include "HelpDialog.h"
#include <QRadioButton>

class HelpRadioButton : public QRadioButton
{
    Q_OBJECT
public:
    explicit HelpRadioButton(QWidget *parent = nullptr);
    void addHelpText(const QString &title, const QString &description);
protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    QString m_title;
    QString m_description;
    HelpDialog *m_helpDialog = nullptr;
};



#endif //DEEPIN_INSTALLER_HELPRADIOBUTTON_H
