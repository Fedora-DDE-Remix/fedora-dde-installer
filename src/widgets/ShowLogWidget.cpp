#include "ShowLogWidget.h"
#include "CommonFunc.h"

#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QDebug>
#include <QScrollBar>
#include <QPushButton>

const int btnWidth = 202;
const int btnHeight = 36;

ShowLogWidget::ShowLogWidget(QWidget *parent) : QWidget(parent)
{
    initUI();
    initConnect();
}

void ShowLogWidget::showLog(const QString &content)
{
    m_installLog->insertPlainText(content);
    m_installLog->moveCursor(QTextCursor::End);
}

void ShowLogWidget::showLogFile(const QString &filepath)
{
    QFile file(filepath);
    if (file.open(QIODevice::ReadOnly)) {
        QString text = file.readAll();
        m_installLog->insertPlainText(text);
        m_installLog->moveCursor(QTextCursor::End);
        file.close();
    } else {
        qDebug() << "Failed to open the log file. file: " << filepath;
    }
}

void ShowLogWidget::saveLogBtnClickedSlot()
{
    Q_EMIT saveLogSignal();
}

void ShowLogWidget::exitInstallBtnClickedSlot()
{
    Q_EMIT exitInstallSignal();
}

void ShowLogWidget::initUI()
{
    //保存
    m_saveLogBtn = new DSuggestButton;
    m_saveLogBtn->setText(::QObject::tr("Save Log"));
    m_saveLogBtn->setMaximumSize(btnWidth, btnHeight);
    m_saveLogBtn->setMinimumSize(btnWidth, btnHeight);
    m_saveLogBtn->setDefault(true);

    // 返回
    m_exitInstallBtn = new QPushButton;
    m_exitInstallBtn->setText(::QObject::tr("Exit", "button"));
    m_exitInstallBtn->setMaximumSize(btnWidth, btnHeight);
    m_exitInstallBtn->setMinimumSize(btnWidth, btnHeight);
    m_exitInstallBtn->setDefault(true);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing(10);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->addWidget(m_exitInstallBtn);
    buttonsLayout->addWidget(m_saveLogBtn);
    QWidget* buttonsWidget = new QWidget;
    buttonsWidget->setLayout(buttonsLayout);

    m_installLog = new QPlainTextEdit(this);
    m_installLog->setContextMenuPolicy(Qt::NoContextMenu);
    m_installLog->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_installLog->horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_installLog->setReadOnly(true);
    m_installLog->setStyleSheet("QPlainTextEdit{background-color:rgba(0, 0, 0, 0.05);"
                                "border-radius:8px;}");

    m_installLog->setMaximumHeight(230);
    m_installLog->setMinimumHeight(60);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    QSize showSize = CommonFunc::getShowSize();
    mainLayout->setContentsMargins(showSize.width() / 6, 0, showSize.width() / 6, 0);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_installLog);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonsWidget, 0, Qt::AlignBottom | Qt::AlignHCenter);

    this->setLayout(mainLayout);
}

void ShowLogWidget::initConnect()
{
    connect(m_saveLogBtn, &QPushButton::clicked, this, &ShowLogWidget::saveLogBtnClickedSlot);
    connect(m_exitInstallBtn, &QPushButton::clicked, this, &ShowLogWidget::exitInstallBtnClickedSlot);
}

void ShowLogWidget::updateTs()
{
    m_saveLogBtn->setText(::QObject::tr("Save Log"));
    m_exitInstallBtn->setText(::QObject::tr("Exit", "button"));
}

void ShowLogWidget::showEvent(QShowEvent *event)
{
    updateTs();
    QWidget::showEvent(event);
}
