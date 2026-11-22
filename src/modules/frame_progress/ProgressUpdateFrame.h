#ifndef PROGRESSUPDATEFREAM_H
#define PROGRESSUPDATEFREAM_H

#include <QWidget>
#include <QPair>
#include <QMap>

class QLabel;
class QHBoxLayout;
class QProgressBar;
class TitleWidget;

class ProgressUpdateFrame : public QWidget
{
    Q_OBJECT
public:
    explicit ProgressUpdateFrame(QWidget *parent = nullptr);
    void setTitle(const QString &content, const QString &stylesheet = "");
    void setProgressTip(const QString &content, const QString &stylesheet = "");
    QProgressBar* progressWidget() { return m_progressWidget; }
    void setProgressStatus(const QString &status);
    void setAnimationsTitle(const QString &title, const QString &tips);
    void updateTS();
signals:
    void startAnime();
private:
    QLabel *m_titleLabel = nullptr;
    QLabel *m_promptLabel = nullptr;
    QProgressBar *m_progressWidget = nullptr;
    TitleWidget *m_titleWidget = Q_NULLPTR;
    QMap<QString, QPair<QString, QString> > m_mapProTrans;
};

#endif // PROGRESSUPDATEFREAM_H
