#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QStackedWidget>
#include "MenuWidget.h"
#include "GameWidget.h"
#include "HighScoreWidget.h"
#include "LevelSelectWidget.h"
#include "PlaneSelectWidget.h" // 引用新头文件

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QStackedWidget *stack;
    MenuWidget *menu;
    GameWidget *game;
    HighScoreWidget *highScore;
    LevelSelectWidget *levelSelect;
    PlaneSelectWidget *planeSelect; // 新增指针
};

#endif // MAINWINDOW_H