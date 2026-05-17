#ifndef SYSTEMINFOTIP_H
#define SYSTEMINFOTIP_H

#include <QFrame>
#include <QLabel>

class SystemInfoTip : public QFrame {
    Q_OBJECT

public:
    explicit SystemInfoTip(QWidget* parent = nullptr);

    void setText(const QString& text);

    void setTextColor(const QString &color);

    void showBottom(const QWidget* widget);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_label = nullptr;
};

#endif
