#ifndef ICONCOMBOBOX_H
#define ICONCOMBOBOX_H

#include <QWidget>
#include <DIconButton>
#include <QComboBox>

DWIDGET_USE_NAMESPACE

class IconComboBox : public QWidget
{
    Q_OBJECT
public:
    explicit IconComboBox(QWidget *parent = nullptr);
    void setIcon(const QIcon &icon);
    QAbstractButton* button();
    QComboBox* comboBox();
    void setMaxVisibleCount(int count);

private:
    DIconButton* m_button = nullptr;
    QComboBox* m_comboBox = nullptr;
    int m_maxVisibleCount = 10;

};

#endif // ICONCOMBOBOX_H
