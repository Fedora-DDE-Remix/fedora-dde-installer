#ifndef HELPLABEL_H
#define HELPLABEL_H

#include <QLabel>

class HelpLabel : public QLabel
{
    Q_OBJECT
public:
    explicit HelpLabel(QWidget *parent = nullptr);

signals:
    void hoverEnter();

    void hoverLeave();
protected:
    bool event(QEvent *e) override;

    void paintEvent(QPaintEvent *event) override;
};

#endif // HELPLABEL_H
