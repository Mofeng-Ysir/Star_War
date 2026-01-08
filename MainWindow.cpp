#include "MainWindow.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    setFixedSize(960, 600);
    setWindowTitle("Qt Space Shooter - Saga Mode");

    stack = new QStackedWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stack);

    menu = new MenuWidget(this);
    levelSelect = new LevelSelectWidget(this);
    game = new GameWidget(this);
    highScore = new HighScoreWidget(this);

    stack->addWidget(menu);        // 0
    stack->addWidget(levelSelect); // 1
    stack->addWidget(game);        // 2
    stack->addWidget(highScore);   // 3

    connect(menu, &MenuWidget::startClicked, this, [this]()
            {
        menu->stopMenu();
        levelSelect->refreshState(); 
        stack->setCurrentWidget(levelSelect); });

    connect(menu, &MenuWidget::historyClicked, this, [this]()
            {
        highScore->refreshScores();
        stack->setCurrentWidget(highScore); });

    connect(levelSelect, &LevelSelectWidget::levelSelected, this, [this](int level)
            {
        stack->setCurrentWidget(game);
        game->startGame(level); });

    connect(levelSelect, &LevelSelectWidget::backClicked, this, [this]()
            {
        menu->startMenu();
        stack->setCurrentWidget(menu); });

    connect(game, &GameWidget::gameEnded, this, [this]()
            {
        game->stopGame();
        menu->startMenu();
        stack->setCurrentWidget(menu); });

    connect(game, &GameWidget::levelWon, this, [this]()
            {
        game->stopGame();
        levelSelect->refreshState(); 
        stack->setCurrentWidget(levelSelect); });

    connect(highScore, &HighScoreWidget::backClicked, this, [this]()
            { stack->setCurrentWidget(menu); });

    menu->startMenu();
}