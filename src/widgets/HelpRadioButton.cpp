#include "HelpRadioButton.h"

HelpRadioButton::HelpRadioButton(QWidget *parent)
{
    m_helpDialog = new HelpDialog(this);
}

void HelpRadioButton::addHelpText(const QString &title, const QString &description)
{
    m_title = title;
    m_description = description;
}


void HelpRadioButton::enterEvent(QEnterEvent *event)
{
    delete m_helpDialog;
    m_helpDialog = new HelpDialog(this);

    m_helpDialog->addHelpText(m_title, m_description);

    m_helpDialog->showCenter(this);
}

void HelpRadioButton::leaveEvent(QEvent *event)
{
    m_helpDialog->close();
}




#include "HelpRadioButton.h"
