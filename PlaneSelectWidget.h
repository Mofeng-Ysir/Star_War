#ifndef PLANESELECTWIDGET_H
#define PLANESELECTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include <QImage>

class PlaneSelectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaneSelectWidget(QWidget *parent = nullptr);
    void refreshUI(); // 刷新金币和解锁状态

signals:
    void backClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void onPlaneClicked(int id);

    QImage bgImg;
    QList<QPushButton *> planeBtns;
    QLabel *infoLabel;      // 显示飞机详情
    QLabel *coinLabel;      // 显示当前金币
    QPushButton *actionBtn; // "装备" 或 "购买" 按钮

    int selectedPreviewId; // 当前选中的（还没装备的）ID
};

#endif // PLANESELECTWIDGET_H