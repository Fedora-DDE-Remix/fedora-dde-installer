#ifndef LISTWIDGET_H
#define LISTWIDGET_H

#include <QWidget>

class QLabel;
class PageIndicator;

class ListWidget : public QWidget {
    Q_OBJECT
public:
    explicit ListWidget(const QString &resource, QWidget *parent = nullptr);

public:
    void setPageCount(int cnt);
    void setCurrentPage(int index);
    void nextPage();
    void backPage();
    void setTitleText(const QString &text);
    int currentIndex();
protected:
    void showEvent(QShowEvent *event) override;

private:
    PageIndicator *m_pageWidgets = nullptr;
    QLabel        *m_titleLabel  = nullptr;
    QLabel        *m_logoLabel   = nullptr;
};

#endif // LISTWIDGET_H
