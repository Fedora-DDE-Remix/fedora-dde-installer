#include "ProgressSuccessFrame.h"
#include "CommonFunc.h"
#include "globals.h"
#include "SaveDeviceWidget.h"
#include "DeviceManager.h"
#include "WarningDialog.h"
#include "ExportFileDialog.h"
#include "AppResultDialog.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <DMessageManager>
#include <QSettings>
#include <QKeyEvent>

const int titleLabelWidth = 540;
const int btnWidth = 202;
const int btnHeight = 36;

ProgressSuccessFrame::ProgressSuccessFrame(QWidget *parent)
    : QWidget(parent), m_saveToUsb(new SaveFileToUsb(this))
{
    initUI();
    initConnection();
}

void ProgressSuccessFrame::installOkSlot()
{
    qInfo() << "reboot System !";
#ifdef QT_DEBUG
    QCoreApplication::exit();
#else
    CommonFunc::rebootSystem();
#endif //QT_DEBUG
}

void ProgressSuccessFrame::exportKeySlot()
{
    QString err = "";
    QString cryptKeyFilePath = "";
    if (CommonFunc::cryptKeyFile(cryptKeyFilePath, err)) {
        m_saveToUsb->setSaveFileList({cryptKeyFilePath});
    } else {
        qWarning() << err;
    }

    QString keyFilePath = CommonFunc::getSettingsString("DI_CRYPT_RECOVERY_KEY");
    FILE *keyFileFP = fopen(keyFilePath.toLocal8Bit().data(), "rb+");
    QString recoveryKeyStr = "";
    if (keyFileFP != nullptr) {
        char recoveryKeyChars[256] = {0};
        fread(recoveryKeyChars, 1, 256, keyFileFP);
        fclose(keyFileFP);
        recoveryKeyStr = QString(recoveryKeyChars);
    }

    m_saveToUsb->setTitleAndMessage(tr("Export Key"),
                                    tr("You have used disk encryption and can export the key to a USB drive for easy password retrieval in the future."));
    m_saveToUsb->setRecoveryKey(recoveryKeyStr);
    m_saveToUsb->beginSaveFile();
}

void ProgressSuccessFrame::initUI()
{
    QLabel *installSuccessIco = new QLabel;
    installSuccessIco->setAlignment(Qt::AlignCenter);
    installSuccessIco->setFixedSize(CommonFunc::getInstallResultIconSize());
    installSuccessIco->setPixmap(CommonFunc::loadSvgImage(":/images/install_success.svg",installSuccessIco->width(), installSuccessIco->height()));

    m_titleLabel = new QLabel;
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setFixedWidth(titleLabelWidth);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T5, QFont::DemiBold);

    m_appDetail = new QWidget;
    m_appDetail->setFixedWidth(titleLabelWidth);
    QHBoxLayout *detailLayout = new QHBoxLayout;
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(0);
    m_appDetail->setLayout(detailLayout);

    DFontSizeManager::SizeType tipSize = DFontSizeManager::T7;
    m_descLabel = new QLabel;
    DFontSizeManager::instance()->bind(m_descLabel, tipSize);

    m_btnDetail = new DCommandLinkButton("");
    m_btnDetail->installEventFilter(this);
    DFontSizeManager::instance()->bind(m_btnDetail, tipSize);
    detailLayout->addStretch();
    detailLayout->addWidget(m_descLabel);
    detailLayout->addWidget(m_btnDetail);
    detailLayout->addStretch();

    m_exportKey = new QPushButton;
    m_exportKey->setMaximumSize(btnWidth, btnHeight);
    m_exportKey->setMinimumSize(btnWidth, btnHeight);
    m_exportKey->setFocusPolicy(Qt::TabFocus);
    connect(m_exportKey, &QPushButton::clicked, this, &ProgressSuccessFrame::exportKeySlot);

    // 退出安装按钮
    m_exitInstallBtn = new DSuggestButton;
    m_exitInstallBtn->setMaximumSize(btnWidth, btnHeight);
    m_exitInstallBtn->setMinimumSize(btnWidth, btnHeight);
    m_exitInstallBtn->setFocusPolicy(Qt::TabFocus);
    m_exitInstallBtn->setDefault(true);
    connect(m_exitInstallBtn, &QPushButton::clicked, this, &ProgressSuccessFrame::installOkSlot);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->addWidget(m_exportKey);
    buttonsLayout->setSpacing(10);
    buttonsLayout->addWidget(m_exitInstallBtn);
    QWidget* buttonsWidget = new QWidget;
    buttonsWidget->setLayout(buttonsLayout);

    m_infoTipLabel = new QLabel;
    m_infoTipLabel->setAlignment(Qt::AlignBottom);
    DFontSizeManager::instance()->bind(m_infoTipLabel, tipSize);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, glayoutTopMargin, 0, 20);
    mainLayout->addSpacing(20);
    mainLayout->addSpacing(60);
    mainLayout->addWidget(installSuccessIco,0, Qt::AlignCenter);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_titleLabel, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(3);
    mainLayout->addWidget(m_appDetail, 0, Qt::AlignHCenter);

    mainLayout->addStretch();
    mainLayout->addWidget(m_infoTipLabel, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(buttonsWidget, 0, Qt::AlignHCenter);
    setLayout(mainLayout);
}

void ProgressSuccessFrame::updateTs()
{
    m_titleLabel->setText(::QObject::tr("Successfully Installed"));
    m_exportKey->setText(::QObject::tr("Export key"));
    m_exitInstallBtn->setText(::QObject::tr("Reboot Now"));
    m_btnDetail->setText(tr("view detail"));
    m_infoTipLabel->setText(::QObject::tr("Click the button below and then remove the installation media immediately"));
    m_saveToUsb->updateTs();
}

void ProgressSuccessFrame::initConnection()
{
    connect(m_btnDetail, &QAbstractButton::clicked, this, [=]{
        AppResultDialog appResult;
        appResult.loadAppResult();
        appResult.exec();
    });
    connect(m_saveToUsb, &SaveFileToUsb::copyDone, [=](bool bSuccess, const QString &devId) {
        DMessageManager::instance()->setContentMargens(this, QMargins(0, 0, 0, 10));
        QIcon messageIcon = bSuccess ? QIcon::fromTheme("icon_ok") : QIcon::fromTheme("icon_fail");
        QString strMessage = bSuccess ? ::QObject::tr("Successfully exported to the \"%1\" device").arg(devId) :
                                        ::QObject::tr("Export failed");
        DMessageManager::instance()->sendMessage(this, messageIcon, strMessage);
    });
}

void ProgressSuccessFrame::showEvent(QShowEvent *event)
{
#ifdef QT_DEBUG
    QSettings settings("/etc/deepin-installer/app-info.ini", QSettings::IniFormat);
#else
    QSettings settings("/target/etc/deepin-installer/app-info.ini", QSettings::IniFormat);
#endif

    int appCount = settings.value("count", 0).toInt();
    if (appCount > 0) {
        m_appDetail->setVisible(true);
        int successCount = 0;
        int failCount = 0;
        auto groups = settings.childGroups();
        for (auto &group : groups) {
            settings.beginGroup(group);
            bool installed = settings.value("installed", false).toBool();
            if (installed) {
                successCount++;
            } else {
                failCount++;
            }
            settings.endGroup();
        }

        if (successCount == appCount) {
            m_descLabel->setText(tr("Selected app(s) installed successfully"));
        } else {
            m_descLabel->setText(tr("%1 app(s) installed successfully, %2 app(s) installation failed")
            .arg(successCount)
            .arg(failCount));
        }
    } else {
        m_appDetail->setVisible(false);
    }

    m_exportKey->setVisible(false);
    updateTs();
    QWidget::showEvent(event);
}

bool ProgressSuccessFrame::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Space || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (obj == m_btnDetail) {
                m_btnDetail->click();
            }
        }
    }
    return QObject::eventFilter(obj, event);
}
