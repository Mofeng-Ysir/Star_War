#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QStackedWidget>
#include "MenuWidget.h"
#include "PlaneSelectWidget.h"
#include "LevelSelectWidget.h"
#include "GameWidget.h"
#include "HighScoreWidget.h"
#include "ShopWidget.h"
#include "EquipmentWidget.h"

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QStackedWidget *stack;
    MenuWidget *menu;
    PlaneSelectWidget *planeSelect;
    LevelSelectWidget *levelSelect;
    GameWidget *game;
    HighScoreWidget *highScore;
    ShopWidget *shop;
    EquipmentWidget *equipment;
};

#endif // MAINWINDOW_H