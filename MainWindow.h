#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QStackedWidget>
#include "MenuWidget.h"
#include "GameWidget.h"
#include "HighScoreWidget.h"
#include "LevelSelectWidget.h"

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
};

#endif // MAINWINDOW_H