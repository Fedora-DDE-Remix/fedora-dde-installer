#include "SelectFileDialog.h"
#include "TitleWidget.h"
#include "globals.h"
#include "UimgFileListView.h"
#include "DeviceManager.h"
#include "CommonFunc.h"
#include <DPaletteHelper>
#include <QHBoxLayout>
#include <QFileInfo>
#include <DPlatformWindowHandle>


ScanUimgWidget::ScanUimgWidget(QWidget *parent) : DWidget(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);

    auto layout = new QVBoxLayout;
    setLayout(layout);

    m_spinner = new DSpinner;
    m_spinner->setFixedSize(64, 64);
    layout->addStretch();
    layout->addWidget(m_spinner, 0,  Qt::AlignCenter);
    layout->addSpacing(10);

    m_label = new DLabel;
    m_label->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_label);

    m_cancelBtn = new DPushButton;
    layout->addStretch();
    layout->addWidget(m_cancelBtn);

    connect(m_cancelBtn, &DPushButton::clicked, this, &ScanUimgWidget::onCancel);
    connect(m_timer, &QTimer::timeout, this, &ScanUimgWidget::onTimeout);
}

ScanUimgWidget::~ScanUimgWidget()
{

}

void ScanUimgWidget::onStart()
{
    m_timer->start();
    m_spinner->start();
    m_fileList.clear();
    DeviceManager::instance()->scanUSBDevices(true);
}

void ScanUimgWidget::onCancel()
{
    m_timer->stop();
    m_spinner->stop();
    DeviceManager::instance()->scanUSBDevices(false);
    Q_EMIT cancel();
}

void ScanUimgWidget::onTimeout()
{
    m_count++;
    m_fileList.clear();
    if (m_count >= 2) {
        m_count=0;
        m_timer->stop();
        m_spinner->stop();
        DeviceManager::instance()->scanUSBDevices(false);
        QList<SDeviceInfo> devList = DeviceManager::instance()->getUSBDeviceInfos();
        for (auto in : devList) {
            QStringList files = CommonFunc::getDirs(QString("%1/clone/").arg(in.mountPoint), {"*.uimg"});
            for (auto &file: files) {
                m_fileList.append(SUimgFileInfo(file, in.device));
            }
        }
        Q_EMIT finish(!m_fileList.isEmpty());
    }
}

QList<SUimgFileInfo> ScanUimgWidget::getUimgFileList() const
{
    return m_fileList;
}

void ScanUimgWidget::updateTs()
{
    m_label->setText(::QObject::tr("Scanning in progress..."));
    m_cancelBtn->setText(::QObject::tr("Cancel"));
}


ScanResultWidget::ScanResultWidget(QWidget *parent) : DWidget(parent)
{
    auto layout = new QVBoxLayout;
    setLayout(layout);
    layout->addStretch();
    m_failImg = new DLabel;
    m_failImg->setPixmap(CommonFunc::hidpiPixmap(":/images/fail.svg", QSize(64, 64)));
    layout->addWidget(m_failImg, 0, Qt::AlignCenter);
    layout->addSpacing(10);

    m_failText = new DLabel;
    m_failText->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_failText);
    layout->addStretch();
    auto hLayout = new QHBoxLayout;
    m_cancelBtn = new DPushButton;
    hLayout->addWidget(m_cancelBtn);
    hLayout->addSpacing(10);

    m_retryBtn = new DPushButton;
    QPalette pal;
    pal.setColor(QPalette::ButtonText, QColor(255,0,0));
    m_retryBtn->setPalette(pal);
    hLayout->addWidget(m_retryBtn);
    layout->addLayout(hLayout);

    connect(m_retryBtn, &DPushButton::clicked, this, &ScanResultWidget::retry);
    connect(m_cancelBtn, &DPushButton::clicked, this, &ScanResultWidget::cancel);
}

ScanResultWidget::~ScanResultWidget()
{

}

void ScanResultWidget::updateTs()
{
    m_failText->setText(::QObject::tr("No uimg file detected"));
    m_cancelBtn->setText(::QObject::tr("Cancel"));
    m_retryBtn->setText(::QObject::tr("Retry"));
}

UImgSelectWidget::UImgSelectWidget(QWidget *parent) : DWidget(parent)
{
    auto layout = new QVBoxLayout;
    setLayout(layout);

    m_fileView = new DListView;
    m_fileView->setItemSpacing(10);
    m_fileView->setMinimumHeight(250);
    m_fileView->setFrameShape(QFrame::NoFrame);
    m_fileView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_fileView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QPalette palette = m_fileView->palette();
    palette.setColor(QPalette::HighlightedText, palette.color(QPalette::Text));
    DPalette dpalette = DPaletteHelper::instance()->palette(m_fileView);
    palette.setColor(QPalette::Highlight, dpalette.color(DPalette::ObviousBackground));
    m_fileView->setPalette(palette);
    m_model = new QStandardItemModel;
    m_fileView->setModel(m_model);
    layout->addWidget(m_fileView);

    auto hLayout = new QHBoxLayout;
    m_cancel = new DPushButton;
    hLayout->addWidget(m_cancel);

    hLayout->addSpacing(10);
    m_ok = new DPushButton;
    hLayout->addWidget(m_ok);
    layout->addLayout(hLayout);

    auto *selectModel = m_fileView->selectionModel();
    connect(selectModel, &QItemSelectionModel::currentChanged, [=](const QModelIndex &current, const QModelIndex &previous){
        if(current.isValid()) {
            DStandardItem *currItem = dynamic_cast<DStandardItem*>(m_model->itemFromIndex(current));
            currItem->actionList(Qt::RightEdge).at(0)->setVisible(true);
        }

        if(previous.isValid()) {
            DStandardItem *preItem = dynamic_cast<DStandardItem*>(m_model->itemFromIndex(previous));
            preItem->actionList(Qt::RightEdge).at(0)->setVisible(false);
        }
    });

    connect(m_cancel, &DPushButton::clicked, this, &UImgSelectWidget::cancel);

    connect(m_ok, &DPushButton ::clicked, [=]{
        if(m_fileView->currentIndex().isValid()) {
            auto *item = m_model->itemFromIndex(m_fileView->currentIndex());
            Q_EMIT uimgSelect(item->data(Qt::UserRole + 100).toString(),
                                    item->data(Qt::UserRole + 101).toString());
        }
    });
}

UImgSelectWidget::~UImgSelectWidget()
{

}

void UImgSelectWidget::resetData(QList<SUimgFileInfo> &files)
{
    m_model->clear();
    for (auto &file: files) {
        QFileInfo fileInfo(file.path);
        DStandardItem *item = new DStandardItem;
        item->setSizeHint(QSize(-1, 46));
        item->setIcon(fileInfo.suffix() == "uimg" ? QIcon(":/images/uimg.svg") : QIcon(":/images/json.svg"));
        item->setText(fileInfo.fileName());
        item->setData(file.path, Qt::UserRole + 100);
        item->setData(file.device, Qt::UserRole + 101);
        item->setData(QVariant::fromValue(QMargins(10, 0, 10, 0)), Dtk::MarginsRole);
        DViewItemAction *rightAction = new DViewItemAction(Qt::AlignVCenter, QSize(20, 20), QSize(20, 20), false);
        QIcon checked = qobject_cast<DStyle*>(QApplication::style())->standardIcon(DStyle::SP_IndicatorChecked);
        rightAction->setIcon(checked);
        rightAction->setVisible(false);
        item->setActionList(Qt::RightEdge, {rightAction});
        m_model->appendRow(item);
    }
}

void UImgSelectWidget::updateTs()
{
    m_cancel->setText(::QObject::tr("Cancel"));
    m_ok->setText(::QObject::tr("OK"));
}

void  UImgFileSelectDialog::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    QTimer::singleShot(0, m_scanUimgWidget, [this]{
       m_stackedLayout->setCurrentIndex(0);
       m_scanUimgWidget->onStart();
    });
}

UImgFileSelectDialog::UImgFileSelectDialog(QWidget *parent):DDialog (parent)
{
    DPlatformWindowHandle *windowHandle = new DPlatformWindowHandle(this);
    windowHandle->setWindowRadius(18);
    setOnButtonClickedClose(false);
    setFixedWidth(580);


    m_stackedLayout = new QStackedLayout;

    m_scanUimgWidget = new ScanUimgWidget;
    m_stackedLayout->addWidget(m_scanUimgWidget);

    m_uimgSelectWidget = new UImgSelectWidget;
    m_stackedLayout->addWidget(m_uimgSelectWidget);

    m_scanResultWidget = new ScanResultWidget;
    m_stackedLayout->addWidget(m_scanResultWidget);

    m_widget = new DWidget;
    m_widget->setLayout(m_stackedLayout);

    addContent(m_widget);

    connect(m_scanUimgWidget, &ScanUimgWidget::cancel, this, &UImgFileSelectDialog::close);
    connect(m_scanResultWidget, &ScanResultWidget::cancel, this, &UImgFileSelectDialog::close);
    connect(m_scanResultWidget, &ScanResultWidget::retry, [this]{
        m_stackedLayout->setCurrentIndex(0);
        m_scanUimgWidget->onStart();
    });

    connect(m_scanUimgWidget, &ScanUimgWidget::finish, [this](bool success){
        if (success) {
            auto files = m_scanUimgWidget->getUimgFileList();
            m_stackedLayout->setCurrentIndex(1);
            m_uimgSelectWidget->resetData(files);
        } else {
            m_stackedLayout->setCurrentIndex(2);
        }
    });

    connect(m_uimgSelectWidget, &UImgSelectWidget::cancel, this, &UImgFileSelectDialog::close);
    connect(m_uimgSelectWidget, &UImgSelectWidget::uimgSelect, [this](const QString &imgPath, const QString &imgDevice){
       close();
       Q_EMIT uimgSelect(imgPath, imgDevice);
    });
}

void UImgFileSelectDialog::updateTs()
{
    setTitle(tr("Please select uimg file"));
    m_uimgSelectWidget->updateTs();
    m_scanUimgWidget->updateTs();
    m_scanResultWidget->updateTs();
}


