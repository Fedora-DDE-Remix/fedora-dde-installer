#include "LicenseDialog.h"
#include "CommonFunc.h"

#include <QVBoxLayout>
#include <QScrollBar>

#include <DLabel>
#include <DScrollArea>

LicenseDialog::LicenseDialog(const QString text, DDialog *parent) : DDialog(parent)
{
    const int dialogWidth = CommonFunc::getShowSize().width() - 144;
    const int dialogHeight = CommonFunc::getShowSize().height() - 38;
    const int scrollAreaWidth = CommonFunc::getShowSize().width() - 184;
    const int scrollAreaHeight = CommonFunc::getShowSize().height() - 124;
    const int scrollAreaWidgetWidth = CommonFunc::getShowSize().width() - 184;
    const int scrollAreaWidgetHeight = CommonFunc::getShowSize().height() - 108;

    this->setFixedSize( dialogWidth, dialogHeight);

    DWidget *MainWidget = new DWidget(this);
    MainWidget->setFixedSize(dialogWidth, dialogHeight);
    //  设置Dialog背景白色
    QPalette paletteMainWidget(MainWidget->palette());
    paletteMainWidget.setColor(QPalette::Window, QColor(247, 247, 247));
    MainWidget->setAutoFillBackground(true);
    MainWidget->setPalette(paletteMainWidget);
    //  用DLabel来显示协议文案
    DLabel *licenseLabel = new DLabel;
    licenseLabel->setText(text);
    licenseLabel->setWordWrap(true);

    //  用DBlurEffectWidget来实现DLabel半透明模糊效果
    DBlurEffectWidget *blurEffectLicenseWidget = new DBlurEffectWidget;
    QHBoxLayout *licenseLabelLayout = new QHBoxLayout;
    licenseLabelLayout->addWidget(licenseLabel);
    blurEffectLicenseWidget->setLayout(licenseLabelLayout);
    blurEffectLicenseWidget->resize(licenseLabel->size());
    blurEffectLicenseWidget->setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    blurEffectLicenseWidget->lower();

    QHBoxLayout *licenseLayout = new QHBoxLayout;
    licenseLayout->setContentsMargins(0, 0, 0, 0);
    licenseLayout->setSpacing(0);
    licenseLayout->addWidget(blurEffectLicenseWidget);
    QWidget *licenseWidget = new QWidget;
    licenseWidget->setLayout(licenseLayout);

    //  设置滚动区域
    DScrollArea * scrollArea = new DScrollArea;
    scrollArea->setWidget(licenseWidget);
    scrollArea->setFixedSize(scrollAreaWidth, scrollAreaHeight);
    scrollArea->setWidgetResizable(true);
    scrollArea->setLineWidth(0);
    scrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    scrollArea->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    scrollArea->horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    vboxLayout->setSpacing(0);
    vboxLayout->addWidget(scrollArea);
    vboxLayout->setAlignment(Qt::AlignmentFlag::AlignCenter);
    //  将滚动区域设置为半透明模糊效果加上圆角
    DBlurEffectWidget *scrollAreaWidget = new DBlurEffectWidget;
    scrollAreaWidget->setFixedSize(scrollAreaWidgetWidth, scrollAreaWidgetHeight);
    //  设置scrollAreaWidget背景灰白色
    QPalette palettescrollAreaWidget(scrollAreaWidget->palette());
    palettescrollAreaWidget.setColor(QPalette::Window, QColor(222, 222, 222));
    scrollAreaWidget->setAutoFillBackground(true);
    scrollAreaWidget->setPalette(palettescrollAreaWidget);
    scrollAreaWidget->setLayout(vboxLayout);
    scrollAreaWidget->setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    scrollAreaWidget->setBlurRectXRadius(8);
    scrollAreaWidget->setBlurRectYRadius(8);
    scrollAreaWidget->lower();

    QVBoxLayout *pLayout = new QVBoxLayout;
    pLayout->addWidget(scrollAreaWidget);
    pLayout->setAlignment(Qt::AlignmentFlag::AlignBottom | Qt::AlignmentFlag::AlignHCenter);
    pLayout->addSpacing(12);
    MainWidget->setLayout(pLayout);
}

LicenseDialog::~LicenseDialog()
{

}
