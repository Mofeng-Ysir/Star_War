#include "MainWindow.h"
#include "DataManager.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    setFixedSize(960, 600);
    setWindowTitle("Qt Space Shooter - Ultimate Edition");

    DataManager::loadData(); // 启动时加载存档

    stack = new QStackedWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stack);

    menu = new MenuWidget(this);
    planeSelect = new PlaneSelectWidget(this);
    levelSelect = new LevelSelectWidget(this);
    game = new GameWidget(this);
    highScore = new HighScoreWidget(this);

    stack->addWidget(menu);        // 0
    stack->addWidget(planeSelect); // 1
    stack->addWidget(levelSelect); // 2
    stack->addWidget(game);        // 3
    stack->addWidget(highScore);   // 4

    // 1. 菜单 -> 战机中心
    connect(menu, &MenuWidget::garageClicked, this, [this]()
            {
        menu->stopMenu();
        planeSelect->refreshUI();
        stack->setCurrentWidget(planeSelect); });

    // 2. 菜单 -> 选关
    connect(menu, &MenuWidget::startClicked, this, [this]()
            {
        menu->stopMenu();
        levelSelect->refreshState(); 
        stack->setCurrentWidget(levelSelect); });

    // 3. 菜单 -> 历史
    connect(menu, &MenuWidget::historyClicked, this, [this]()
            {
        highScore->refreshScores();
        stack->setCurrentWidget(highScore); });

    // 4. 战机中心 -> 菜单
    connect(planeSelect, &PlaneSelectWidget::backClicked, this, [this]()
            {
        menu->startMenu();
        stack->setCurrentWidget(menu); });

    // 5. 选关 -> 游戏
    connect(levelSelect, &LevelSelectWidget::levelSelected, this, [this](int level)
            {
        stack->setCurrentWidget(game);
        game->startGame(level); });

    // 6. 选关 -> 菜单
    connect(levelSelect, &LevelSelectWidget::backClicked, this, [this]()
            {
        menu->startMenu();
        stack->setCurrentWidget(menu); });

    // 7. 游戏 -> 结束
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

    // 8. 历史 -> 菜单
    connect(highScore, &HighScoreWidget::backClicked, this, [this]()
            { stack->setCurrentWidget(menu); });

    menu->startMenu();
}