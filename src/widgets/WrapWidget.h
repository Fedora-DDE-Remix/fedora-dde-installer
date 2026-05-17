#ifndef WRAPWIDGET_H
#define WRAPWIDGET_H

#include <QWidget>
#include <QVBoxLayout>

class WrapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WrapWidget(QWidget *headWidget, QWidget *contentWidget, QWidget *parent = nullptr);

signals:
    void headClicked(const bool isVisible);
};

#endif // WRAPWIDGET_H
