#include <QVBoxLayout>
#include <QListView>

#include "IconComboBox.h"


IconComboBox::IconComboBox(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_comboBox = new QComboBox;
    m_comboBox->setVisible(false);
    m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_comboBox->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_comboBox);

    m_button = new DIconButton;
    layout->addWidget(m_button);

    connect(m_button, &QAbstractButton::clicked, [this]{
        m_comboBox->showPopup();
        int count = m_comboBox->count();
        QWidget* popup = this->findChild<QFrame*>();

        if (popup == nullptr) {
            return;
        }

        if (count <= 0) {
            return;
        } else if (count <= m_maxVisibleCount) {
            popup->setFixedHeight(m_comboBox->view()->sizeHintForRow(0) * count + 16);
        } else {
            popup->setFixedHeight(m_comboBox->view()->sizeHintForRow(0) * m_maxVisibleCount + 16);
        }

        popup->move(m_button->mapToGlobal(QPoint(0, popup->height() * -1 - 10)));
        popup->show();
    });


}

void IconComboBox::setIcon(const QIcon &icon)
{
    m_button->setIcon(icon);
}

QAbstractButton* IconComboBox::button()
{
    return m_button;
}

QComboBox* IconComboBox::comboBox()
{
    return m_comboBox;
}

void IconComboBox::setMaxVisibleCount(int count)
{
    if (count > 0) {
        m_maxVisibleCount = count;
    }
}
