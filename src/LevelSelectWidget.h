#ifndef LEVELSELECTWIDGET_H
#define LEVELSELECTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QList>
#include <QImage>

class LevelSelectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LevelSelectWidget(QWidget *parent = nullptr);
    void refreshState();

signals:
    void levelSelected(int level);
    void backClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<QPushButton *> levelBtns;
    QImage bgImg;
};

#endif // LEVELSELECTWIDGET_H