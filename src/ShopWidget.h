#ifndef SHOPWIDGET_H
#define SHOPWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QImage>

class ShopWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ShopWidget(QWidget *parent = nullptr);
    void refreshUI();

signals:
    void backClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void buyItem(int id);

    QImage bgImg;
    QLabel *coinLabel;
    QPushButton *buyBtn;
    QWidget *shopContainer;
    QVBoxLayout *shopLayout;
};

#endif // SHOPWIDGET_H