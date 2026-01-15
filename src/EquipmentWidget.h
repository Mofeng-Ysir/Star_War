#ifndef EQUIPMENTWIDGET_H
#define EQUIPMENTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QImage>

class EquipmentWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EquipmentWidget(QWidget *parent = nullptr);
    void refreshUI();

signals:
    void backClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void onSlotClicked(int type);
    void onInventoryItemClicked(int id);
    void updateSlotUI(int type, QPushButton *btn, QLabel *lbl);

    QImage bgImg;
    // 3个部位的按钮
    QPushButton *btnCore, *btnArmor, *btnEngine;
    QLabel *lblCore, *lblArmor, *lblEngine;

    QWidget *inventoryContainer;
    QVBoxLayout *inventoryLayout; // 现在编译器认识它了

    int currentSelectedSlotType;
};

#endif // EQUIPMENTWIDGET_H