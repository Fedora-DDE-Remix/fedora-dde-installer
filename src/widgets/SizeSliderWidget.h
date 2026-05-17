#ifndef SIZESLIDERWIDGET_H
#define SIZESLIDERWIDGET_H

#include <QWidget>
#include <QLabel>
#include <DClipEffectWidget>

DWIDGET_USE_NAMESPACE

class RoundWidget:public QWidget
{
    Q_OBJECT
public:
    explicit RoundWidget(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *ev) override;
};

class SliderHandle:public QFrame
{
    Q_OBJECT
public:
    explicit SliderHandle(QWidget *parent = nullptr);
Q_SIGNALS:
    void pressMove(int x);
protected:
    void paintEvent(QPaintEvent *ev) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    bool m_isPress;
    QPoint m_lastPos;
};

class SizeSliderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SizeSliderWidget(QWidget *parent = nullptr);

    void initPartitionSize(int total, int sysMin, int other);
public slots:
    void setSysSize(int sys);
Q_SIGNALS:
    void sizeChange(int sysSize, int dataSize, int otherSize);
protected:
    void showEvent(QShowEvent *event) override;
private slots:
    void onHandleMove(int distance);
private:
    int getPartitionWidth(int partSize);

    void updateHandlePos();

    int getPartitionSize(int partWidth);

    void updateLabelWidth(int distance);
private:
    SliderHandle *m_handle;
    QLabel *m_sysLabel;
    QLabel *m_dataLabel;
    QLabel *m_otherLabel;
    DClipEffectWidget *m_clip;
    int m_totalSize;
    int m_sysSize;
    int m_sysMinSize;
    int m_sysMaxSize;
    int m_dataSize;
    int m_otherSize;
    int m_minXPos;
    int m_maxXPos;
};

#endif // SIZESLIDERWIDGET_H
