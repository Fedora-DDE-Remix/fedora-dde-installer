#include "ShowLogWidget.h"
#include "CommonFunc.h"

#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QDebug>
#include <QScrollBar>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>

const int btnWidth = 202;
const int btnHeight = 36;
const int qrCodeBtnSize = 36;

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

void ShowLogWidget::switchQRAndLogBtnSlot()
{
    if (m_installLog->isVisible()) {
        m_qrCodeBtn->setIcon(QIcon(":/images/failed_data.svg"));
        m_logAndQRCodeWidget->setCurrentWidget(m_qrcodePicWidget);
        m_installLog->hide();
        m_qrcodePicWidget->show();
    } else {
        m_qrCodeBtn->setIcon(QIcon(":/images/failed_qr.svg"));
        m_logAndQRCodeWidget->setCurrentWidget(m_installLog);
        m_installLog->show();
        m_qrcodePicWidget->hide();
    }
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

    QVBoxLayout* qrcodePicLayout = new QVBoxLayout;
    qrcodePicLayout->setContentsMargins(0,0,0,0);
    QLabel *qrcodePic = new QLabel;
    QSize qrPicSize = CommonFunc::getQRPicSize();
    QString qrCodePath = (CommonFunc::getCurrentOs() == OSType::Community) ? ":/images/qr_code_deepin.svg" : ":/images/qr_code.svg";
    qrcodePic->setPixmap(CommonFunc::loadSvgImage(qrCodePath, qrPicSize.width(), qrPicSize.height()));
    qrcodePicLayout->addWidget(qrcodePic, 0, Qt::AlignHCenter);
    m_qrcodePicWidget = new QWidget;
    m_qrcodePicWidget->setLayout(qrcodePicLayout);
    m_qrcodePicWidget->setObjectName("qrcodePicWidget");
    m_qrcodePicWidget->setStyleSheet("QWidget#qrcodePicWidget{background-color:rgba(0, 0, 0, 0.05);"
                                     "border-radius:8px;}");
    m_qrcodePicWidget->hide();

    m_logAndQRCodeWidget = new QStackedWidget();
    m_logAndQRCodeWidget->setMaximumHeight(230);
    m_logAndQRCodeWidget->setMinimumHeight(60);
    m_logAndQRCodeWidget->addWidget(m_installLog);
    m_logAndQRCodeWidget->addWidget(m_qrcodePicWidget);
    m_logAndQRCodeWidget->setCurrentWidget(m_installLog);

    m_qrCodeBtn = new QPushButton(this);
    m_qrCodeBtn->setIconSize(QSize(24, 24));
    m_qrCodeBtn->setFixedSize(qrCodeBtnSize,qrCodeBtnSize);
    m_qrCodeBtn->setIcon(QIcon(":/images/failed_qr.svg"));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    QSize showSize = CommonFunc::getShowSize();
    mainLayout->setContentsMargins(showSize.width() / 6, 0, showSize.width() / 6, 0);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_logAndQRCodeWidget);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonsWidget, 0, Qt::AlignBottom | Qt::AlignHCenter);

    this->setLayout(mainLayout);
}

void ShowLogWidget::initConnect()
{
    connect(m_saveLogBtn, &QPushButton::clicked, this, &ShowLogWidget::saveLogBtnClickedSlot);
    connect(m_exitInstallBtn, &QPushButton::clicked, this, &ShowLogWidget::exitInstallBtnClickedSlot);
    connect(m_qrCodeBtn, &QPushButton::clicked, this, &ShowLogWidget::switchQRAndLogBtnSlot);
}

void ShowLogWidget::updateTs()
{
    m_saveLogBtn->setText(::QObject::tr("Save Log"));
    m_exitInstallBtn->setText(::QObject::tr("Exit", "button"));
}

void ShowLogWidget::showEvent(QShowEvent *event)
{
    updateTs();
    if (m_qrCodeBtn != nullptr) {
        QSize showSize = CommonFunc::getShowSize();
        int xPos = CommonFunc::getShowSize().width() - ((showSize.width() / 6) * 2) - m_qrCodeBtn->width() - 10;
        m_qrCodeBtn->move(m_logAndQRCodeWidget->x() + xPos, m_logAndQRCodeWidget->y() + 10);
        m_qrCodeBtn->raise();
    }
}
