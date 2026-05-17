#ifndef LISTVIEWWIDGETITEM_H
#define LISTVIEWWIDGETITEM_H

#include <QWidget>

class QLabel;
class QCheckBox;
class QRadioButton;

class ComponentsListWidgetItem : public QWidget {
    Q_OBJECT

public:
    enum class SelectType
    {
        RadioButtonSelect,
        CheckBoxSelect,
        NoType
    };

    explicit ComponentsListWidgetItem(QWidget *parent = nullptr, SelectType selecttype = SelectType::NoType);

public:
    void setShowText(const QString &title, const QString &content);
    void setSelect(bool isselect);
    bool isSelected();
    int getHeight();
    void updateSize(int width);
    QRadioButton* radioButton() { return m_radioSelect; }

private:
    void initUI();

private:
    int m_realHeight = 0;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_contentLabel = nullptr;
    QRadioButton* m_radioSelect = nullptr;
    QCheckBox *m_selectBox = nullptr;
    SelectType m_selectType;
};

#endif // LISTVIEWWIDGETITEM_H
