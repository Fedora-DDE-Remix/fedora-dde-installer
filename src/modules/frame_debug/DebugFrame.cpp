#include "DebugFrame.h"

#include <QHBoxLayout>
#include <QFont>


DebugShellFrame::DebugShellFrame(QWidget *parent):
    FramePluginInterface(parent)
{
    hideBackButton();
    hideCloseButton();

    m_mainLayout = new QHBoxLayout();
    m_mainLayout->setContentsMargins(10, 10, 10, 10);

    this->setLayout(m_mainLayout);
}

void DebugShellFrame::showEvent(QShowEvent *event)
{
    if (m_termWidget == nullptr) {
        m_termWidget = new QTermWidget(this);
        QFont termFont = this->font();
        termFont.setFamily("Monospace");
        m_termWidget->setTerminalFont(termFont);
        m_termWidget->setScrollBarPosition(QTermWidget::NoScrollBar);
        m_termWidget->setColorScheme("WhiteOnBlack");
        m_termWidget->sendText("cd /\n");
        m_termWidget->sendText("/bin/bash\n");
        m_termWidget->sendText("export TERM=xterm\n");
        m_termWidget->sendText("clear\n");
        m_mainLayout->removeWidget(m_termWidget);
        m_mainLayout->addWidget(m_termWidget);

        connect(m_termWidget, &QTermWidget::finished, this, [=](){
            m_termWidget->deleteLater();
            m_termWidget = nullptr;
            close();
        });
    }

    m_termWidget->resize(this->size());
    m_termWidget->show();

    FramePluginInterface::showEvent(event);
}

void DebugShellFrame::resizeEvent(QResizeEvent *event)
{
    if (m_termWidget != nullptr) {
        m_termWidget->resize(this->size());
    }
    FramePluginInterface::resizeEvent(event);
}