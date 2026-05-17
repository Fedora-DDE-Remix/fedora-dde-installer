#include "CMainWindow.h"

#include "PluginManager.h"
#include "CommonFunc.h"
#include "ListWidget.h"
#include "FramePluginInterface.h"

#include <QPushButton>
#include <QStackedLayout>
#include <QPluginLoader>
#include <QDebug>
#include <QVBoxLayout>
#include <QWidget>
#include <QShortcut>
#include <QEvent>
#include <DFontSizeManager>
#include <QStackedWidget>
#include <QResizeEvent>
#include "PageIndicator.h"
#include <QMouseEvent>

MainWindow::MainWindow(QWidget *parent):QMainWindow (parent),
    m_framePartition(nullptr),
    m_framePrepare(nullptr),
    m_frameApplication(nullptr)
{
    setWindowFlag(Qt::FramelessWindowHint);
    initUI();

    // 开启安装器系统调试窗口
    if (CommonFunc::getSettingsBool("DI_USE_DEBUG_FRAME")) {
        auto *shortcut = new QShortcut(QKeySequence("Ctrl+L"), this);
        connect(shortcut, &QShortcut::activated, this, &MainWindow::debugShell);
    }
}

void MainWindow::initUI()
{
    QWidget *centerWidget = new QWidget;
    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
    //head
    QHBoxLayout *headLayout = new QHBoxLayout;
    headLayout->setContentsMargins(60, 0, 60, 0);
    QWidget *logoWidget = new QWidget;
    QVBoxLayout *logoLayout = new QVBoxLayout;
    logoLayout->setContentsMargins(0, 40, 0, 0);
    logoLayout->setSpacing(0);
    logoWidget->setLayout(logoLayout);
    QLabel *logoLabel = new QLabel;
    QPixmap logo(CommonFunc::getSystemLogo());
    logoLabel->setPixmap(logo.scaledToHeight(24, Qt::SmoothTransformation));
    logoLayout->addWidget(logoLabel, 0, Qt::AlignTop | Qt::AlignLeft);

    QWidget *titleWidget = new QWidget;
    QVBoxLayout *titleLayout = new QVBoxLayout;
    if (CommonFunc::isBigScreen()) {
        titleLayout->setContentsMargins(0, 0, 0, 40);
    } else {
        titleLayout->setContentsMargins(0, 0, 0, 10);
    }
    
    titleLayout->setSpacing(0);
    titleWidget->setLayout(titleLayout);
    m_titleLabel = new QLabel;
    if (CommonFunc::isBigScreen()) {
        m_titleLabel->setContentsMargins(0, 40, 0, 0);
    } else {
        m_titleLabel->setContentsMargins(0, 10, 0, 0);
    }
    

    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T2);
    m_titleLabel->setStyleSheet("color:white");
    titleLayout->addWidget(m_titleLabel, 0, Qt::AlignBottom | Qt::AlignHCenter);

    headLayout->addWidget(logoWidget);
    headLayout->addWidget(titleWidget);
    headLayout->addWidget(new QWidget);
    //mid
    QHBoxLayout *midLayout = new QHBoxLayout;
    midLayout->setContentsMargins(0, 0, 0, 0);
    m_pluginWidget = new QStackedWidget;
    m_pluginWidget->setFixedSize(CommonFunc::getShowSize());

    midLayout->addWidget(m_pluginWidget, 0, Qt::AlignCenter);
    //bottom
    QHBoxLayout *bottomLayout = new QHBoxLayout;
    m_pageWidgets = new PageIndicator;
    m_pageWidgets->setFixedWidth(CommonFunc::getShowSize().width());
    bottomLayout->addWidget(m_pageWidgets, 0, Qt::AlignCenter);
    connect(m_pluginWidget, &QStackedWidget::currentChanged, m_pageWidgets, &PageIndicator::setCurrentPage);
    //
    main_layout->addLayout(headLayout);
    main_layout->addLayout(midLayout);
    main_layout->addLayout(bottomLayout);
    centerWidget->setLayout(main_layout);
    setCentralWidget(centerWidget);
}

void MainWindow::setPluginSize()
{
    m_pluginWidget->setFixedSize(CommonFunc::getShowSize());
    m_pageWidgets->setFixedWidth(CommonFunc::getShowSize().width());
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEv = dynamic_cast<QMouseEvent*>(event);
        if(mouseEv && mouseEv->button() == Qt::RightButton) {
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showPlugins(const QStack<SPluginInfo> &plugs)
{
    qDebug() << "show Plugins count: " << plugs.size();

    if (!plugs.isEmpty()) {
        nextLists = plugs;

        qDebug() << "page count: " << nextLists.size();
        listWidget->setPageCount(nextLists.size());
        qDebug() << "current page: " << 0;
        listWidget->setCurrentPage(0);
    }

    qDebug() << "show full screen...";
    this->plugins(nextLists);  // 加载主流程插件
}

void MainWindow::loadPlugins(const QString &pluginType)
{
    auto plugins = CommonFunc::getPlugins(pluginType);
    int totalSize = 0;
    //load plugins
    while (!plugins.isEmpty()) {
        qDebug() << "load plugin:" << plugins.top().path;
        PluginInterface *plu = PluginsManager::instance()->getPlugin(plugins.pop().path);
        FramePluginInterface* frame = dynamic_cast<FramePluginInterface*>(plu);
        if (frame && frame->load()) {
            totalSize++;
            if(frame->frameName() == gFramePartition) {
                m_framePartition = frame;
            } else if(frame->frameName() == gFramePrepare) {
                m_framePrepare = frame;
            } else if(frame->frameName() == gFrameApplication) {
                m_frameApplication = frame;
            }

            m_pluginWidget->addWidget(frame);
            if(m_pluginWidget->count() == 1) {
                frame->hideBackButton();
            }

            initPluginConnection(frame, m_pluginWidget);
        }
    }

    m_pageWidgets->setPageCount(totalSize);
    m_pageWidgets->setCurrentPage(0);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->updateTs();
    }

    return QWidget::changeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

void MainWindow::initTranslator(const QString &locale)
{
    QString tsFile = QString("%1/deepin-installer-%2.qm").arg(DI_I18N_DIR_ENV).arg(locale);
    qInfo() << "set translator to:" << tsFile;
    m_translator->load(tsFile);
}

void MainWindow::initPluginConnection(FramePluginInterface *frame, QStackedWidget *pluginWidget)
{
    connect(frame, &FramePluginInterface::next, [=]{
        int index = pluginWidget->currentIndex();
        auto *currFrame = qobject_cast<FramePluginInterface*>(pluginWidget->currentWidget());
        if(currFrame->frameName() == gFramePartition &&
            CommonFunc::isGhost() &&
            m_framePrepare != nullptr) {
            pluginWidget->setCurrentWidget(m_framePrepare);
        } else if (currFrame->frameName() == gFramePartition &&
                   CommonFunc::getSettingsString("DI_PARTITION_TYPE") == QString::number(SaveData) &&
                   m_frameApplication != nullptr) {
            pluginWidget->setCurrentIndex(index + 2);
        } else {
            pluginWidget->setCurrentIndex(index + 1);
        }
    });

    connect(frame, &FramePluginInterface::back, [=]{
        int index = pluginWidget->currentIndex();
        auto *currFrame = qobject_cast<FramePluginInterface*>(pluginWidget->currentWidget());
        if(currFrame->frameName() == gFramePrepare &&
            CommonFunc::isGhost() &&
            m_framePartition != nullptr) {
            pluginWidget->setCurrentWidget(m_framePartition);
        } else if (currFrame->frameName() == gFrameUserInfo &&
                   CommonFunc::getSettingsString("DI_PARTITION_TYPE") == QString::number(SaveData) &&
                   m_frameApplication != nullptr) {
            pluginWidget->setCurrentIndex(index - 2);
        } else {
            pluginWidget->setCurrentIndex(index - 1);
        }

        qInfo() << "current Index:" << pluginWidget->currentIndex();
    });

    connect(frame, &FramePluginInterface::onChangeLanguage, this, &MainWindow::initTranslator);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPixmap bgPixmap(CommonFunc::getMainGifAnimations());
    painter.drawPixmap(0, 0, width(), height(), bgPixmap);

    QMainWindow::paintEvent(event);
}

void MainWindow::plugins(QStack<SPluginInfo> &lists)
{
    if (lists.isEmpty()) {
        return;
    }

    PluginInterface *plu = PluginsManager::instance()->getPlugin(lists.top().path);
    FramePluginInterface* frame = dynamic_cast<FramePluginInterface*>(plu);
    if (frame && frame->load()) {
        // 加载下一个frame插件
        disconnect(frame, &FramePluginInterface::next, nullptr, nullptr);
        connect(frame, &FramePluginInterface::next, listWidget, &ListWidget::nextPage);
        connect(frame, &FramePluginInterface::next, frame, &FramePluginInterface::hide);
        connect(frame, &FramePluginInterface::next, [=]{
            if (!nextLists.isEmpty()) {
                backLists.push(nextLists.pop());
                plugins(nextLists);
            }
        });

        // 加载上一个frame插件
        disconnect(frame, &FramePluginInterface::back, nullptr, nullptr);
        connect(frame, &FramePluginInterface::back, listWidget, &ListWidget::backPage);
        connect(frame, &FramePluginInterface::back, frame, &FramePluginInterface::hide);
        connect(frame, &FramePluginInterface::back, [=]{
            if (!backLists.isEmpty()) {
                plugins(backLists);
                nextLists.push(backLists.pop());
            }
        });

        frame->setParent(this);
        frame->show();
        if(listWidget->currentIndex() == 0) {
            frame->hideBackButton();
        }
    } else {
        nextLists.pop();
        plugins(nextLists);
    }
}

void MainWindow::debugShell()
{
    if (m_debugShellFrame == nullptr) {
        PluginInterface *plu = PluginsManager::instance()->getPathPlugin(QString("%1/libDebugShellFrame.so").arg(CommonFunc::getSystemEnv(DI_INSTALL_MODULE_DIR_ENV)));
        if(plu && plu->load()) {
            m_debugShellFrame = dynamic_cast<FramePluginInterface *>(plu);
            m_debugShellFrame->setParent(this);
            m_debugShellFrame->setWindowTitle(QString(::QObject::tr("Debug Shell")));
            m_debugShellFrame->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
            m_debugShellFrame->resize(600, 400);
            m_debugShellFrame->move(0,0);
        }
    }

    if (m_debugShellFrame != nullptr) {
        m_debugShellFrame->activateWindow();
        m_debugShellFrame->isHidden() ? m_debugShellFrame->show() : m_debugShellFrame->hide();
    }
}

void MainWindow::updateTs()
{
    // 试用模式时不显示这段标题
    if (qApp->applicationName() != "deepin-installer-live-config")
        m_titleLabel->setText(QString(::QObject::tr("Welcome to install %1")).arg(CommonFunc::isNameDeepin()));
}

