#include <DFrame>
#include <DCheckBox>
#include <DTipLabel>
#include <DFontSizeManager>
#include <QFormLayout>
#include <DListView>
#include <DMessageManager>
#include <DPaletteHelper>
#include <QItemSelectionModel>
#include <QScreen>
#include "PasswdDialog.h"
#include "globals.h"
#include "TitleWidget.h"
#include "PasswordManager.h"
#include "CommonFunc.h"
#include "HelpLabel.h"
#include "DeviceManager.h"
#include "GhostListView.h"

static const QString TMP_MOUNTPOINT = "startusb";

PasswdDialog::PasswdDialog(QWidget *parent):
    DDialog (parent)
{
    setOnButtonClickedClose(false);
    setFixedWidth(440);
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    m_model = new QStandardItemModel;

    InitPage();
    addContent(m_mainWidget);

    m_nextBtn->setEnabled(false);
    initConnection();
    m_autoDecryptDlg->updateTs();
    setFixedHeight(320);

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

void PasswdDialog::initConnection()
{
    connect(m_nextBtn,  &QAbstractButton::clicked, this, &PasswdDialog::checkPassword);
    connect(m_cancelBtn, &QAbstractButton::clicked, this, &DDialog::reject);
    connect(m_mainWidget, &QStackedWidget::currentChanged, [=](int index){
        if(index == 1) {
            this->setFixedHeight(360);
            this->adjustSize();
            this->m_timer->start();
            DeviceManager::instance()->scanUSBDevices(true);
        }
    });
    connect(m_timer, &QTimer::timeout, this, &PasswdDialog::refreshUsbDevice);
    connect(this, &DDialog::finished, [=]{
        QDir tmp("/media");
        if(tmp.exists(TMP_MOUNTPOINT)) {
            QString output;
            if(!CommonFunc::command("umount", {tmp.absoluteFilePath(TMP_MOUNTPOINT)}, output)) {
                qWarning() << "umount tmp mountpoint:" << TMP_MOUNTPOINT << " failed:" << output;
            }

            tmp.rmdir(TMP_MOUNTPOINT);
        }

        m_timer->stop();
        DeviceManager::instance()->scanUSBDevices(false);
    });
    connect(m_safeKeyEdit, &DLineEdit::textChanged, [this](){
        m_nextBtn->setEnabled(!m_safeKeyEdit->text().isEmpty() && !m_ensureEdit->text().isEmpty());
        m_safeKeyEdit->hideAlertMessage();
        m_safeKeyEdit->setAlert(false);
    });

    connect(m_safeKeyEdit, &DLineEdit::editingFinished, [this]() {
        if (!m_ensureEdit->hasFocus()) {
            return ;
        }
        QString err;
        if (!PasswordManager::instance()->checked(QString(), m_safeKeyEdit->text().trimmed(), HigherLevel, err)) {
            m_safeKeyEdit->showAlertMessage(err);
            m_safeKeyEdit->setAlert(true);
            return;
        }
    });

    connect(m_ensureEdit, &DLineEdit::textChanged, [this](){
        m_nextBtn->setEnabled(!m_safeKeyEdit->text().isEmpty() && !m_ensureEdit->text().isEmpty());
        m_ensureEdit->hideAlertMessage();
        m_ensureEdit->setAlert(false);
    });

    connect(this, &DDialog::visibleChanged, [=](bool visible){
        if(visible) {
            m_safeKeyEdit->setFocus();
        }
    });

    connect(m_listView->selectionModel(),  &QItemSelectionModel::currentChanged, [=](const QModelIndex &current, const QModelIndex &previous){
        Q_UNUSED(previous);
        QStandardItem *currItem = m_model->itemFromIndex(current);
        if(currItem) {
            this->m_eptipLabel->setText(tr("Key will be exported to the \"%1\" device").arg(currItem->data(Qt::UserRole + 100).toString()));
        } else {
            this->m_eptipLabel->setText(tr("No key will be exported"));
        }
    });
}

void PasswdDialog::removeOldDevice(const QString &id)
{
    int row = 0;
    for (; row < m_model->rowCount(); ++row) {
        auto *item = m_model->item(row);
        QString deviceId = item->data(Qt::UserRole + 100).toString();
        if(deviceId == id) {
            break;
        }
    }

    if(row < m_model->rowCount()) {
        m_model->removeRow(row);
    }
}

void PasswdDialog::addUsbDevice(const QString &devName, const QString &devPath, const QString &devId, const QString &mountPath)
{
    DPalette palette = DPaletteHelper::instance()->palette(m_listView);
    QStandardItem *item = new QStandardItem;
    item->setIcon(QIcon::fromTheme(":/images/drive-harddisk-32px.svg"));
    item->setText(devName);
    item->setSizeHint(QSize(-1, 49));
    item->setData(palette.color(DPalette::ItemBackground), Qt::BackgroundRole);
    item->setData(palette.color(DPalette::ObviousBackground), Qt::UserRole + 110);
    item->setData(palette.color(DPalette::TextTips), Qt::UserRole + 111);

    item->setData(devId, Qt::UserRole + 100);
    item->setData(mountPath, Qt::UserRole + 101);
    m_model->appendRow(item);
}

void PasswdDialog::checkPassword()
{
    QString err;
    QString strSafeKeyText = m_safeKeyEdit->text().trimmed();
    QString strEnsureText = m_ensureEdit->text().trimmed();
    if(strSafeKeyText.isEmpty()) {
        m_safeKeyEdit->hideAlertMessage();
        m_safeKeyEdit->setAlert(false);
        return;
    } else if (!PasswordManager::instance()->checked(QString(), strSafeKeyText, HigherLevel, err)) {
        m_safeKeyEdit->showAlertMessage(err);
        m_safeKeyEdit->setAlert(true);
        return;
    }

    m_ensureEdit->hideAlertMessage();
    if (strSafeKeyText != strEnsureText) {
        m_ensureEdit->showAlertMessage(::QObject::tr("Passwords do not match"));
        m_ensureEdit->setAlert(true);
    } else {
        m_ensureEdit->hideAlertMessage();
        m_ensureEdit->setAlert(false);
        m_tipsLabel->setText(::QObject::tr("Take care of your password, otherwise, all your data will be lost"));
        m_strPasswd = m_ensureEdit->text().trimmed().toLocal8Bit().toBase64();
        m_mainWidget->setCurrentIndex(1);
    }
}

void PasswdDialog::refreshUsbDevice()
{
    QStringList tempList;
    QList<SDeviceInfo> deviceInfos = DeviceManager::instance()->getUSBDeviceInfos();
    std::transform(deviceInfos.begin(), deviceInfos.end(), std::back_inserter(tempList), [](const SDeviceInfo &info){return info.device;});
    auto iter = m_devList.begin();
    for (; iter != m_devList.end(); ) {
        if(!tempList.contains(*iter)) {
            removeOldDevice(*iter);
            iter = m_devList.erase(iter);
        } else {
            ++iter;
        }
    }
    //
    for (SDeviceInfo deviceInfoItem : deviceInfos) {
        QString output;
        if (deviceInfoItem.isStartDevice && deviceInfoItem.mountPoint.startsWith("/usr/lib/live")) {
            QDir dir("/media");
            if (!dir.exists(TMP_MOUNTPOINT)) {
                dir.mkdir(TMP_MOUNTPOINT);
                bool ok = CommonFunc::command("mount",
                                              QStringList() << "-v" << "--bind" << deviceInfoItem.mountPoint << dir.absoluteFilePath(TMP_MOUNTPOINT), output);
                if (!ok) {
                    qWarning() << output;
                } else {
                    deviceInfoItem.mountPoint = dir.absoluteFilePath(TMP_MOUNTPOINT);
                }
            }
        }

        // 过滤掉挂载位置不可写的设备
        if (!CommonFunc::mountPointWriteable(deviceInfoItem.mountPoint)) {
            continue;
        }

        if(!m_devList.contains(deviceInfoItem.device)) {
            addUsbDevice(deviceInfoItem.name, deviceInfoItem.device, deviceInfoItem.device, deviceInfoItem.mountPoint);
            m_devList.append(deviceInfoItem.device);
        }
    }
}

void PasswdDialog::exportKey()
{
    QString destPath;
    QString devId;
    if(m_listView->selectionModel()->hasSelection()) {
        auto index = m_listView->selectionModel()->selectedIndexes().first();
        DStandardItem *item = reinterpret_cast<DStandardItem*>(m_model->itemFromIndex(index));
        devId = item->data(Qt::UserRole + 100).toString();
        destPath = item->data(Qt::UserRole + 101).toString();
    }

    QString err = "";
    QString cryptKeyFilePath = "";
    QString keyPath = CommonFunc::getSettingsString("DI_CRYPT_RECOVERY_KEY");
    if (CommonFunc::cryptKeyFile(cryptKeyFilePath, err)) {
        keyPath = cryptKeyFilePath;
    } else {
        qWarning() << "crypt key file error:" << err;
    }

    if(!destPath.isEmpty()) {
        QString log = "";
        QString cmd = QString("cp -rf %1 \"%2\"").arg(keyPath, destPath);
        if ( !CommonFunc::command("/bin/bash", {"-c", cmd}, log) ) {
            qCritical() << "cp log error cmd : " << cmd << " err : " << log;
            DMessageManager::instance()->setContentMargens(parentWidget(), QMargins(0, 0, 0, 10));
            DMessageManager::instance()->sendMessage(parentWidget(), QIcon::fromTheme("icon_warning"), ::QObject::tr("Export failed"));
            return;
        } else {
            qInfo() << QString("Command: %1 succeed.").arg(cmd);
            DMessageManager::instance()->setContentMargens(parentWidget(), QMargins(0, 0, 0, 10));
            DMessageManager::instance()->sendMessage(parentWidget(), QIcon::fromTheme("icon_success"), ::QObject::tr("Successfully exported to the \"%1\" device").arg(devId));
            CommonFunc::sync();
        }
    }

    accept();
}

QString PasswdDialog::getPasswd()
{
    return m_strPasswd;
}

void PasswdDialog::setAutoDecryptVisible(bool isVisible)
{
    m_decryptWidget->setVisible(isVisible);
}

bool PasswdDialog::autoDecryptIsChecked()
{
    return m_autoDecrypt->isChecked();
}

void PasswdDialog::InitPage()
{
    m_mainWidget = new QStackedWidget;
    QWidget *pagePwd = new QWidget;
    //init password page
    InitPasswordPage(pagePwd);
    m_mainWidget->addWidget(pagePwd);

    QWidget *pageExport = new QWidget;
    //init export page
    InitExportPage(pageExport);
    m_mainWidget->addWidget(pageExport);
}

void PasswdDialog::InitPasswordPage(QWidget *passwdPage)
{
    m_autoDecryptDlg = new AutoDecryptDialog;
    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    passwdPage->setLayout(contentLayout);
    //title
    QVBoxLayout *titleLayout = new QVBoxLayout;
    DLabel *titleLabel = new DLabel;
    titleLabel->setText(::QObject::tr("Disk Encryption"));
    DFontSizeManager::instance()->bind(titleLabel, DFontSizeManager::T5, QFont::DemiBold);
    titleLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    //password
    QFormLayout *layoutPasswd = new QFormLayout;
    layoutPasswd->setContentsMargins(0, 0, 0, 0);
    layoutPasswd->setSpacing(10);
    QRegularExpression rx = QRegularExpression("[^ \\\\ \\\"']*");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(rx);
    m_safeKeyEdit = new DPasswordEdit(this);
    m_safeKeyEdit->lineEdit()->setValidator(validator);
    m_safeKeyEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_safeKeyEdit->setPlaceholderText(::QObject::tr("Please input password"));
    m_safeKeyEdit->setContentsMargins(0, 0, 0, 0);
    //check password
    m_ensureEdit = new DPasswordEdit(this);
    m_ensureEdit->lineEdit()->setValidator(validator);
    m_ensureEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_ensureEdit->setPlaceholderText(::QObject::tr("Enter the password again"));
    m_ensureEdit->setContentsMargins(0, 0, 0, 0);
    layoutPasswd->addRow(QString("%1:").arg(::QObject::tr("Security Key")), m_safeKeyEdit);
    layoutPasswd->addRow(QString("%1:").arg(::QObject::tr("Confirm Security Key")), m_ensureEdit);

    //tip
    QHBoxLayout *layoutTip = new QHBoxLayout;
    layoutTip->setContentsMargins(0, 0, 0, 0);
    layoutTip->setSpacing(0);
    m_tipsLabel = new DLabel(this);
    m_tipsLabel->setForegroundRole(DPalette::TextWarning);
    m_tipsLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    m_tipsLabel->setMinimumHeight(30);
    m_tipsLabel->setWordWrap(true);
    m_tipsLabel->setText(tr("Using disk encryption may affect disk performance"));
    layoutTip->addWidget(m_tipsLabel);
    //auto decrypt
    HelpLabel *helpLabel = new HelpLabel;
    m_decryptWidget = new QWidget;
    QHBoxLayout *subLayout = new QHBoxLayout;
    QHBoxLayout *decryptLayout = new QHBoxLayout;
    decryptLayout->setSpacing(0);
    m_autoDecrypt = new DCheckBox(::QObject::tr("Auto decrypt"));
    decryptLayout->addWidget(m_autoDecrypt);
    decryptLayout->addSpacing(5);
    decryptLayout->addWidget(helpLabel);
    m_decryptWidget->setLayout(decryptLayout);
    m_decryptWidget->setVisible(CommonFunc::isTPM());
    subLayout->addWidget(m_decryptWidget, 0, Qt::AlignHCenter);
    //button
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(10);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    m_cancelBtn = new DPushButton(::QObject::tr("Cancel", "button"));
    m_cancelBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_nextBtn = new DSuggestButton(::QObject::tr("Next", "button"));
    m_nextBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_nextBtn);

    contentLayout->addLayout(titleLayout);
    contentLayout->addSpacing(20);
    contentLayout->addLayout(layoutPasswd);
    contentLayout->addSpacing(10);
    contentLayout->addLayout(subLayout);
    contentLayout->addStretch();
    contentLayout->addLayout(layoutTip);
    contentLayout->addSpacing(10);
    contentLayout->addLayout(btnLayout);

    connect(helpLabel, &HelpLabel::hoverEnter, [=]{
        m_autoDecryptDlg->showCenter(helpLabel);
    });
    connect(helpLabel, &HelpLabel::hoverLeave, [=]{
        m_autoDecryptDlg->hide();
    });
}

void PasswdDialog::InitExportPage(QWidget *exportPage)
{
    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    exportPage->setLayout(contentLayout);
    //title
    QVBoxLayout *titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);
    titleLayout->setContentsMargins(10, 0, 10, 0);
    DLabel *titleLabel = new DLabel;
    titleLabel->setText(tr("Export Key"));
    DTipLabel *msgLabel = new DTipLabel;
    msgLabel->setText(tr("You have used disk encryption and can export the key to a USB drive for easy password retrieval in the future."));
    msgLabel->setWordWrap(true);
    msgLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    msgLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    DFontSizeManager::instance()->bind(titleLabel, DFontSizeManager::T5, QFont::DemiBold);
    titleLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    titleLayout->addWidget(msgLabel);
    //
    QString recoverKeyPath = CommonFunc::getSettingsString("DI_CRYPT_RECOVERY_KEY");
    QString strKey;
    QFile keyFile(recoverKeyPath);
    if(!keyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "open recovery key file failed.";
    } else {
        strKey.append(keyFile.readAll());
    }

    QHBoxLayout *keyLayout = new QHBoxLayout;
    keyLayout->setContentsMargins(10, 0, 10, 0);
    m_keyLabel = new DLabel(tr("Key: ") + strKey);
    m_keyLabel->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_keyLabel, DFontSizeManager::T6, QFont::DemiBold);
    keyLayout->addWidget(m_keyLabel);
    //
    QHBoxLayout *viewLayout = new QHBoxLayout;
    viewLayout->setContentsMargins(0, 0, 0, 0);
    m_listView = new QListView;
    m_listView->setMinimumHeight(102);
    m_listView->setIconSize(QSize(32, 32));
    m_listView->setFrameShape(DListView::NoFrame);
    m_listView->setSelectionMode(DListView::SingleSelection);
    m_listView->setEditTriggers(DListView::NoEditTriggers);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setSizeAdjustPolicy(DListView::AdjustToContents);
    m_listView->setSpacing(1);
    m_listView->viewport()->setAutoFillBackground(false);
    m_listView->setAutoFillBackground(false);
    m_listView->setBackgroundRole(m_listView->viewport()->backgroundRole());
    m_listView->setItemDelegate(new DiskDelegate);
    m_listView->setModel(m_model);
    viewLayout->addWidget(m_listView);
    //
    QHBoxLayout *tipLayout = new QHBoxLayout;
    tipLayout->setContentsMargins(0, 0, 0, 0);
    m_eptipLabel = new DTipLabel(tr("No key will be exported"));
    tipLayout->addWidget(m_eptipLabel);
    //
    DPushButton *btnOk = new DPushButton(::QObject::tr("Confirm", "button"));
    btnOk->setFixedHeight(36);
    btnOk->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    contentLayout->addLayout(titleLayout);
    contentLayout->addSpacing(8);
    contentLayout->addLayout(keyLayout);
    contentLayout->addSpacing(10);
    contentLayout->addStretch();
    contentLayout->addLayout(viewLayout);
    contentLayout->addSpacing(10);
    contentLayout->addLayout(tipLayout);
    contentLayout->addSpacing(10);
    contentLayout->addWidget(btnOk);
    connect(btnOk, &DPushButton::clicked, this, &PasswdDialog::exportKey);
}
