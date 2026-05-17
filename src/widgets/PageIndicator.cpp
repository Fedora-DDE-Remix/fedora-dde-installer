#include "PageIndicator.h"
#include <CommonFunc.h>

#include <QLabel>
#include <QDebug>

PageIndicator::PageIndicator(QWidget *parent):
    QFrame (parent)
{
    m_mainLayout = new QHBoxLayout;
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(m_mainLayout);
    this->setFixedHeight(4);
}

void PageIndicator::setPageCount(int cnt)
{
    // 清理布局
    if (!m_pageWidgets.isEmpty()) {
        m_pageWidgets.clear();
        CommonFunc::deleteLayout(this->layout());
    }

    // 添加控件
    m_mainLayout->addStretch();
    for (int i = 0; i < cnt; i++) {
        QLabel *label = new QLabel(this);
        m_mainLayout->addWidget(label);
        m_mainLayout->addSpacing(10);
        m_pageWidgets.append(label);
        this->setSelectStyle(i, false);
    }
    m_mainLayout->addStretch();
}

void PageIndicator::setCurrentPage(int index)
{
    if (!m_pageWidgets.isEmpty() && index >= 0 && index < m_pageWidgets.size()) {
        this->setSelectStyle(m_currentPage, false); // 还原上一次选择的控件的样式
        this->setSelectStyle(index, true);          // 设置当前选择的控件的样式
        m_currentPage = index;
    }
}

void PageIndicator::next()
{
    // m_currentPage值在setCurrentPage中统一修改
    setCurrentPage(m_currentPage + 1);
}

void PageIndicator::back()
{
    // m_currentPage值在setCurrentPage中统一修改
    setCurrentPage(m_currentPage - 1);
}

void PageIndicator::setSelectStyle(int index, bool isSelect)
{
    if (!m_pageWidgets.isEmpty() && index >= 0 && index < m_pageWidgets.size()) {
        QLabel *label = m_pageWidgets.at(index);
        if (nullptr != label) {
            const int width = 50;
            const int height = 4;

            label->setFixedSize(width, height);
            isSelect ? label->setStyleSheet("QLabel{border-radius:2px;background-color:rgba(255, 255, 255, 255);}")
                     : label->setStyleSheet("QLabel{border-radius:2px;background-color:rgba(255, 255, 255, 102);}");
        }
    }
}

void PageIndicator::setFixedHeight(const int height)
{
    QFrame::setFixedHeight(height);
}
