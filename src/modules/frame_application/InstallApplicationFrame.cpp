#include "InstallApplicationFrame.h"
#include "TitleWidget.h"
#include "CommonFunc.h"
#include "globals.h"
#include "AppDelegate.h"
#include <QSettings>

Q_DECLARE_METATYPE(QMargins)

InstallApplicationFrame::InstallApplicationFrame(QWidget *parent): FramePluginInterface (parent)
{
    m_frameName = gFrameApplication;
    m_titleWidget = new TitleWidget;
    //
    m_btnBox = new DButtonBox;
    //
    m_appStack = new QStackedWidget;
    m_appStack->setFixedWidth(CommonFunc::isBigScreen() ? 700 : 550);
    m_appStack->setMinimumHeight(CommonFunc::isBigScreen() ? 500 : 260);

    //next btn
    m_nextBnt = new DSuggestButton;
    m_nextBnt->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBnt->setDefault(true);

    m_contentLayout->addWidget(m_titleWidget);
    m_contentLayout->addSpacing(CommonFunc::isBigScreen() ? 20 : 10);
    m_contentLayout->addWidget(m_btnBox, 0, Qt::AlignHCenter);
    m_contentLayout->addSpacing(CommonFunc::isBigScreen() ? 40 : 10);
    m_contentLayout->addWidget(m_appStack, 0, Qt::AlignHCenter);
    m_contentLayout->addStretch();
    m_contentLayout->addWidget(m_nextBnt, 0, Qt::AlignHCenter);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());
    //
    initConnection();
}

bool InstallApplicationFrame::load()
{
    QString optionalPath;
#ifdef QT_DEBUG
    optionalPath = "/home/uos/Desktop/deb_optional";
#else
    optionalPath = QString("%1/oem/deb_optional").arg(CommonFunc::getSystemEnv("DI_LIVE_DIR_ENV"));
#endif
    qInfo() << "optionalPath=" << optionalPath;
    QDir dir(optionalPath);
    if (!dir.exists()) {
        return false;
    }

    QList<QFileInfo> list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs);

    addApplicationItem(list);
    return  true;
}

void InstallApplicationFrame::showEvent(QShowEvent *event)
{
    if(m_btnBox->buttonList().size() > 0) {
        m_btnBox->buttonList().at(0)->click();
    }

    FramePluginInterface::showEvent(event);
}

void InstallApplicationFrame::updateTs()
{
    m_titleWidget->setTitleLabelProperty(tr("Selected apps"), "");
    m_titleWidget->setTipsLabelProperty(tr("Selected the app(s) as needed and it will be automatically installed while installing system"),
                                        "font-size:16px;font-weight:Medium;");
    m_nextBnt->setText(::QObject::tr("Next"));
}

void InstallApplicationFrame::initConnection()
{
    connect(m_btnBox, &DButtonBox::buttonClicked, [=](QAbstractButton* btn){
        int id = m_btnBox->id(btn);
        m_appStack->setCurrentIndex(id);
    });

    connect(m_nextBnt, &QPushButton::clicked, [=] () {
        QSettings settings("/etc/fedora-dde-installer/app-info.ini", QSettings::IniFormat);
        settings.clear();
        int count = 0;
        for (int i = 0; i < m_appStack->count(); i++) {
            auto widget = m_appStack->widget(i);
            if (widget != nullptr) {
                AppViewWidget *appWidget = dynamic_cast<AppViewWidget*>(widget);
                auto appInfos = appWidget->getSelectedAppInfo();
                for (auto &info : appInfos) {
                    settings.beginGroup(QString("App%1").arg(count));
                    settings.setValue("appName", info.appName);
                    settings.setValue("appPath", info.appPath);
                    settings.setValue("appIcon", info.appIcon);
                    settings.setValue("installed", info.installed);
                    settings.endGroup();
                    count++;
                }
            }
        }
        settings.setValue("count", count);
        Q_EMIT next();
    });
}

void InstallApplicationFrame::addApplicationItem(QList<QFileInfo> &category)
{
    QList<DButtonBoxButton*> btnList;
    for (auto &c: category) {
        btnList.append(new DButtonBoxButton(c.fileName()));
        AppViewWidget *appWidget = new AppViewWidget;
        appWidget->initModelData(c.filePath());
        m_appStack->addWidget(appWidget);
    }

    m_btnBox->setButtonList(btnList, true);
    for (int i = 0; i < btnList.size(); ++i) {
        m_btnBox->setId(btnList[i], i);
    }
}
