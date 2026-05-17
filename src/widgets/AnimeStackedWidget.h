#ifndef ANIMESTACKEDWIDGET_H
#define ANIMESTACKEDWIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>

class AnimeStackedWidget:public QStackedWidget
{
    Q_OBJECT
public:
    explicit AnimeStackedWidget(QWidget *parent = nullptr);
public Q_SLOTS:
    void setCurrentIndex(int index);
private:
    QLabel *m_label;
};

#endif // ANIMESTACKEDWIDGET_H
