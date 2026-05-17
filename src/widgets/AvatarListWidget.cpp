#include "AvatarListWidget.h"
#include "AvatarButton.h"
#include "ScrollAreaWidget.h"
#include "CommonFunc.h"

#include <QButtonGroup>
#include <DBackgroundGroup>
#include <QKeyEvent>

namespace {
    const int kWindowWidth = 440;
    const int kWindowHeight = 430;
    const int kScrollAreaMinHeigh = 310;

    const int kColumnCounts = 4;
}

AvatarListWidget::AvatarListWidget(QWidget *parent)
    : DDialog (parent)
    , m_btnGroup(new QButtonGroup)
{
    m_gridLayout = new QGridLayout;
    m_gridLayout->setSpacing(0);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setColumnStretch(4, 1);

    DFrame* widget = new DFrame;
    widget->setFocusPolicy(Qt::NoFocus);
    widget->setStyleSheet(""
                                  "QWidget:focus {"
                                  "border: 2px solid;"
                                  "border-radius:5px;"
                                  "border-color:rgb(1, 128, 255)"
                                  "}");
    widget->setFrameShape(QFrame::Shape::NoFrame);
    widget->setObjectName("widget");

    widget->setLayout(m_gridLayout);

    ScrollAreaWidget* scroll = new ScrollAreaWidget(widget);
    scroll->setFixedWidth(kWindowWidth - 20);
    scroll->setFixedHeight(kScrollAreaMinHeigh);
    scroll->setStyleSheet("QScrollArea{background-color:transparent}");
    scroll->viewport()->setStyleSheet("background-color:transparent");
    scroll->widget()->setStyleSheet("background-color:transparent");

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll, 0, Qt::AlignHCenter);
    layout->addSpacing(20);

    setFixedSize(kWindowWidth, kWindowHeight);

    QWidget *wdg = new QWidget;
    wdg->setFocusPolicy(Qt::TabFocus);
    wdg->setStyleSheet(""
                          "QWidget:focus {"
                          "border: 2px solid;"
                          "border-radius:5px;"
                          "border-color:rgb(1, 128, 255)"
                          "}");
    wdg->setLayout(layout);

    addContent(wdg);
    addButton(::QObject::tr("Cancel", "button"));
    addButton(::QObject::tr("Confirm", "button"), true, DDialog::ButtonRecommend);
    m_cancelBnt = getButton(0);
    m_okBnt = getButton(1);
    connect(m_okBnt, &QAbstractButton::clicked, this, [=] {
        AvatarButton* curBtn = static_cast<AvatarButton *>(m_btnGroup->checkedButton());
        if (curBtn != nullptr) {
            emit this->selectAvatar(curBtn->avatar());
        }
    });

    updateTs();
}

void AvatarListWidget::setAvatars(const QStringList &avatars)
{
    int row = 0;
    int column = 0;
    int id = 0;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_btnGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), this, &AvatarListWidget::onButtonGroupClicked);
#else
    connect(m_btnGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &AvatarListWidget::onButtonGroupClicked);
#endif

    for (QStringList::const_iterator it = avatars.constBegin(); it != avatars.constEnd(); ++it, ++id) {
        AvatarButton* avatarBtn = new AvatarButton(this, true);
        avatarBtn->updateIcon(*it);

        m_gridLayout->addWidget(avatarBtn, row, column, Qt::AlignHCenter);

        column += 1;
        if (column >= kColumnCounts) {
            column = 0;
            row += 1 ;
        }

        m_btnGroup->addButton(avatarBtn, id);
    }
}

void AvatarListWidget::setCurrentAvatar(const QString &path)
{
    for (auto &it : m_btnGroup->buttons()) {
        AvatarButton* curBtn = static_cast<AvatarButton *>(it);
        if (curBtn == nullptr) {
            continue;
        }

        if (curBtn->avatar() == path) {
            curBtn->setChecked(true);
            curBtn->update();

            return;
        }
    }
}

void AvatarListWidget::showEvent(QShowEvent *event)
{
    updateTs();

    DDialog::showEvent(event);
}

void AvatarListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Right) {
        auto id = m_btnGroup->checkedId();
        id++;
        if (m_btnGroup->button(id) != nullptr) {
            m_btnGroup->button(id)->setChecked(true);
        }
    } else if (event->key() == Qt::Key_Left){
        auto id = m_btnGroup->checkedId();
        id--;
        if (m_btnGroup->button(id) != nullptr) {
            m_btnGroup->button(id)->setChecked(true);
        }
    }
}

void AvatarListWidget::onButtonGroupClicked(int id)
{
    AvatarButton* btn = static_cast<AvatarButton *>(m_btnGroup->button(id));
    if (btn != nullptr) {
        btn->update();
    }
}

void AvatarListWidget::updateTs()
{
    m_okBnt->setText(::QObject::tr("Confirm", "button"));
    m_cancelBnt->setText(::QObject::tr("Cancel", "button"));
}
