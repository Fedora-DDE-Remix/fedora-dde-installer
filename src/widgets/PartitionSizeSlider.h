/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PARTITIONSIZESLIDERH
#define PARTITIONSIZESLIDERH

#include <QFrame>
#include <DSlider>
#include <DLineEdit>
DWIDGET_USE_NAMESPACE

class QLineEdit;
class QLabel;
class SystemInfoTip;
class PartitionSizeSlider : public QFrame
{
    Q_OBJECT

public:
    explicit PartitionSizeSlider(QWidget* parent = Q_NULLPTR);

    // Get current partition size, in mib.
    qint64 value();

    // Get maximum size, in mib.
    qint64 maximum();

    // Get minimum size, in mib.
    qint64 minimum();

    void paintEvent(QPaintEvent* event) override;

signals:
    // Emitted when slide |value| is updated by user or by program.
    void valueChanged(qint64 value);

public slots:
    // Set maximum partition size, in mib.
    void setMaximum(qint64 maximumSize);

    // Set minimum value of slider, in mib.
    // |minimumSize| shall be in range [0, mMaximumSize].
    void setMinimum(qint64 minimumSize);

    // Set value of slider to |size|, in mib.
    // |size| shall be in range [mMinimumSize, mMaximumSize].
    void setValue(qint64 size);

private:
    void initConnection();
    void initUI();

    DSlider* mSlider = Q_NULLPTR;
    DLineEdit* mEditor = Q_NULLPTR;
    QLabel* m_sizelabel = Q_NULLPTR;
};

#endif  // PARTITIONSIZESLIDERH
