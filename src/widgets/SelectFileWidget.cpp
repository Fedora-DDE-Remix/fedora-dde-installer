#include "SelectFileWidget.h"
#include "CommonFunc.h"
#include "SelectFileDialog.h"
#include "DeviceManager.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QFileDialog>
#include <QStandardPaths>
#include <QToolTip>

#include <DPalette>

SelectFileWidget::SelectFileWidget(QWidget *parent)
    : DFrame(parent), m_pathLabel(new DTipLabel), m_btnOpen(new DIconButton)
{
    m_pathLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_pathLabel->setMinimumWidth(440);
    m_btnOpen->setIcon(QIcon(":/images/select_btn.svg"));
    m_btnOpen->setFlat(true);
    m_btnOpen->setIconSize(QSize(24, 24));

    m_uimgDlg = new UImgFileSelectDialog;

    connect(m_uimgDlg, &UImgFileSelectDialog::uimgSelect, [=](const QString &imgPath, const QString &imgDev){
        qInfo() << "select image file:" << imgPath << ", device:" << imgDev;
        m_filePath = imgPath;
        m_fileDevice = imgDev;
        QFileInfo imgFileInfo(imgPath);
        m_pathLabel->setText(imgFileInfo.fileName());
        m_pathLabel->setToolTip(imgPath);
        Q_EMIT this->textChanged(m_filePath);
    });


    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(10, 0, 10, 0);
    mainLayout->addSpacing(0);
    mainLayout->addWidget(m_pathLabel);
    mainLayout->addStretch();
    mainLayout->addWidget(m_btnOpen);
    this->setLayout(mainLayout);

    connect(m_btnOpen, &QAbstractButton::clicked, [=]{
        m_uimgDlg->exec();
    });

    m_btnOpen->installEventFilter(this);
}

void SelectFileWidget::updateTs()
{
    QFileInfo imgFileInfo(m_filePath);
    m_pathLabel->setText(m_filePath.isEmpty() ? tr("Please select uimg file") : imgFileInfo.fileName());
    m_uimgDlg->updateTs();
}

QString SelectFileWidget::getSelectFilePath()
{
    return m_filePath;
}

QString SelectFileWidget::getSelectDevice()
{
    return m_fileDevice;
}


bool SelectFileWidget::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {
        case QEvent::Enter: {
            if (watched == m_btnOpen) {
                setCursor(QCursor(Qt::PointingHandCursor));
            }
        }
        break;
        case QEvent::Leave: {
            if (watched == m_btnOpen) {
                setCursor(QCursor(Qt::ArrowCursor));
            }
        }
        break;
        default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}


