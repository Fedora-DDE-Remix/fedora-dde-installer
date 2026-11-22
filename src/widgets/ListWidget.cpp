#include "ListWidget.h"
#include "CommonFunc.h"
#include "PageIndicator.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QDebug>
#include <QApplication>
#include <QScreen>

ListWidget::ListWidget(const QString &resource, QWidget *parent):
    QWidget (parent)
{
    // 标题栏
    QFont font;
    // 设置的字体大小是跟随像素的
    font.setPixelSize(26);
    m_titleLabel = new QLabel(this);
    m_titleLabel->setFixedWidth(700);
    m_titleLabel->setFixedHeight(56);
    m_titleLabel->setStyleSheet("color:white");
    m_titleLabel->setFont(font);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // 进度栏显示
    m_pageWidgets = new PageIndicator(this);
    m_pageWidgets->setFixedWidth(CommonFunc::getShowSize().width());

    // 动画显示
    QLabel *animationWidget = new QLabel(this);
    animationWidget->setScaledContents(true);
    if (CommonFunc::getSettingsBool("DI_IS_ENABLE_MOVIE_CONFIG")) {
        QMovie *movie = new QMovie(resource);
        movie->start();
        animationWidget->setMovie(movie);
    }
    else {
        animationWidget->setPixmap(resource);
    }

    // logo
    m_logoLabel = new QLabel(this);
    m_logoLabel->setFixedSize(106, 22);
    m_logoLabel->setPixmap(CommonFunc::hidpiPixmap(CommonFunc::getSystemLogo(), m_logoLabel->size()));
    m_logoLabel->setVisible(false);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->addWidget(animationWidget);

    this->setLayout(main_layout);
}

void ListWidget::setPageCount(int cnt)
{
    m_pageWidgets->setPageCount(cnt);
}

void ListWidget::setCurrentPage(int index)
{
    m_pageWidgets->setCurrentPage(index);
}

void ListWidget::nextPage()
{
    m_pageWidgets->next();
}

void ListWidget::backPage()
{
    m_pageWidgets->back();
}

void ListWidget::setTitleText(const QString &text)
{
    m_titleLabel->setText(text);
}

int ListWidget::currentIndex()
{
    return m_pageWidgets->getCurrentPage();
}

void ListWidget::showEvent(QShowEvent *event)
{
    QRect mm = QApplication::primaryScreen()->geometry();
    int outShowSizeHeight = (mm.height() - CommonFunc::getShowSize().height()) / 2;// 主窗体上下区域的高度
    m_titleLabel->move((mm.width() - m_titleLabel->width()) / 2, (outShowSizeHeight - m_titleLabel->height()) / 2);
    m_titleLabel->raise();
    m_titleLabel->show();
    qInfo() << "m_titleLabel = " << m_titleLabel->pos();

    m_pageWidgets->move((mm.width() - m_pageWidgets->width()) / 2, (mm.height() - outShowSizeHeight) + (outShowSizeHeight - m_pageWidgets->height()) / 2);
    m_pageWidgets->raise();
    m_pageWidgets->show();
    qInfo() << "m_pageWidgets = " << m_pageWidgets->pos();

    // 试用模式时不显示Logo
    if (qApp->applicationName() != "fedora-dde-installer-live-config") {
        CommonFunc::isBigScreen() ? m_logoLabel->move(61, 41) : m_logoLabel->move(30, 20);
        m_logoLabel->raise();
        m_logoLabel->show();
    }

    return QWidget::showEvent(event);
}

