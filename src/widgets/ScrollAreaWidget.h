#ifndef SCROLLAREAWIDGET_H
#define SCROLLAREAWIDGET_H

#include <QScrollArea>

class ScrollAreaWidget : public QScrollArea {
    Q_OBJECT
public:
    explicit ScrollAreaWidget(QWidget *showWidget);
};

#endif // SCROLLAREAWIDGET_H
