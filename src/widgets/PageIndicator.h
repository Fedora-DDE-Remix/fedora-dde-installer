#ifndef PAGEINDICATOR_H
#define PAGEINDICATOR_H

#include <QFrame>

class QLabel;
class QHBoxLayout;

class PageIndicator : public QFrame
{
public:
    explicit PageIndicator(QWidget *parent = nullptr);

    void setPageCount(int cnt);

    int getCurrentPage()
    {
        return m_currentPage;
    }
public slots:
    void setCurrentPage(int index);
protected:
    virtual void setSelectStyle(int index, bool isSelect);

    // 对外屏蔽设置大小的方法，防止被误用
    void setFixedHeight(const int height);
    void setFixedSize(const QSize &size) = delete;

public slots:
    void next();
    void back();

private:
    QList<QLabel*> m_pageWidgets;
    int m_currentPage = 0;
    QHBoxLayout *m_mainLayout = nullptr;
};

#endif // PAGEINDICATOR_H
