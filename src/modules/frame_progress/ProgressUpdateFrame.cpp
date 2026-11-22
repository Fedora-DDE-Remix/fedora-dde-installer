#include "ProgressUpdateFrame.h"
#include "AnimationWidget.h"
#include "CommonFunc.h"
#include "TitleWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <DIconButton>
#include <QLabel>


DWIDGET_USE_NAMESPACE

ProgressUpdateFrame::ProgressUpdateFrame(QWidget *parent)
    : QWidget (parent)
{
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    m_titleWidget->setMinimumHeight(90);
    QHBoxLayout *titleMainLayout = new QHBoxLayout;
    titleMainLayout->addWidget(m_titleWidget);

    //　显示动画
    AnimationWidget *animationWidget = new AnimationWidget(this);
    animationWidget->setAnimations(CommonFunc::getInstallAnimations());
    connect(this, &ProgressUpdateFrame::startAnime, animationWidget, &AnimationWidget::startAnimation);
    connect(animationWidget, &AnimationWidget::animationChange, this, [=](const QString &pictureName) {
        for (auto &key: m_mapProTrans.keys()) {
            if(pictureName.contains(key)) {
                setAnimationsTitle(m_mapProTrans[key].first, m_mapProTrans[key].second);
                break;
            }
        }
    });

    // 上一个动画按钮
    DIconButton *backAnimationButton = new DIconButton(this);
    backAnimationButton->setIcon(QIcon::fromTheme("go-previous"));
    backAnimationButton->setIconSize(QSize(20, 20));
    backAnimationButton->setStyleSheet("background-color:rgba(255, 255, 255, 0);");
    backAnimationButton->setFocusPolicy(Qt::NoFocus);
    QVBoxLayout *backAnimationLayout = new QVBoxLayout;
    backAnimationLayout->addStretch();
    backAnimationLayout->addWidget(backAnimationButton);
    backAnimationLayout->addStretch();
    connect(backAnimationButton, &DIconButton::clicked, animationWidget, &AnimationWidget::backAnimation);

    // 下一个动画按钮
    DIconButton *nextAnimationButton = new DIconButton(this);
    nextAnimationButton->setIcon(QIcon::fromTheme("go-next"));
    nextAnimationButton->setIconSize(QSize(20, 20));
    nextAnimationButton->setStyleSheet("background-color:rgba(255, 255, 255, 0);");
    nextAnimationButton->setFocusPolicy(Qt::NoFocus);
    QVBoxLayout *nextAnimationLayout = new QVBoxLayout;
    nextAnimationLayout->addStretch();
    nextAnimationLayout->addWidget(nextAnimationButton);
    nextAnimationLayout->addStretch();
    connect(nextAnimationButton, &DIconButton::clicked, animationWidget, &AnimationWidget::nextAnimation);

    // 动画主布局
    QHBoxLayout *animationLayout = new QHBoxLayout;
    animationLayout->setContentsMargins(0, 0, 0, 0);
    animationLayout->setSpacing(0);
    animationLayout->addLayout(backAnimationLayout, 1);
    animationLayout->addSpacing(15);
    animationLayout->addWidget(animationWidget, 15);
    animationLayout->addSpacing(15);
    animationLayout->addLayout(nextAnimationLayout, 1);

    // 提示信息
    m_titleLabel = new QLabel;
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPixelSize(22);
    m_titleLabel->setFont(titleFont);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addStretch();
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();
    m_titleLabel->setVisible(CommonFunc::isBigScreen());

    m_promptLabel = new QLabel;
    m_promptLabel->setAlignment(Qt::AlignCenter);
    QHBoxLayout *promptLayout = new QHBoxLayout;
    promptLayout->addStretch();
    promptLayout->addWidget(m_promptLabel);
    promptLayout->addStretch();

    // 进度显示
    m_progressWidget = new QProgressBar(this);
    m_progressWidget->setValue(0);
    m_progressWidget->setAlignment(Qt::AlignCenter);  // 对齐方式

    QHBoxLayout *progressLayout = new QHBoxLayout;
    progressLayout->setContentsMargins(100, 0, 100, 0);
    progressLayout->addWidget(m_progressWidget);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(titleMainLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(animationLayout);
    mainLayout->addSpacing(CommonFunc::isBigScreen() ? 10 : 0);
    mainLayout->addLayout(titleLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(progressLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(promptLayout);
    mainLayout->addSpacing(CommonFunc::isBigScreen() ? 60 : 8);

    this->setLayout(mainLayout);
}

void ProgressUpdateFrame::setProgressStatus(const QString &status)
{
    auto locale = CommonFunc::getSettingsString("DI_LOCALE");
    if (locale == "ug_CN") { //维语特殊处理
        m_progressWidget->setFormat(QString("%1%v").arg(QString("%1%").arg(status)));
    } else {
        m_progressWidget->setFormat(QString("%1 %v%").arg(status));
    }
}

void ProgressUpdateFrame::setAnimationsTitle(const QString &title, const QString &tips)
{
    m_titleWidget->setTitleLabelProperty(title, "");
    m_titleWidget->setTipsLabelProperty(tips, "font-size:16px;font-weight:Medium;");
}

void ProgressUpdateFrame::updateTS()
{
    m_promptLabel->setText(tr("After the system is installed, you will create your account here without restarting."));
    m_mapProTrans = {
        {"01", {tr("Fedora DDE Remix"), tr("Fedora Linux with the Deepin desktop environment.")}},
        {"02", {tr("A familiar desktop"), tr("Your applications and settings, together in DDE.")}},
        {"03", {tr("Your account comes next"), tr("Choose your username, password, computer name and region after installation.")}}
    };
    setAnimationsTitle(tr("Installing Fedora DDE Remix"), tr("Keep this window open while the system is copied to your disk."));
}
