#include "MainWindow.h"
#include "DataManager.h"
#include <QVBoxLayout>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    resize(960, 600);
    setMinimumSize(960, 600);
    setWindowTitle("Qt Space Shooter - Ultimate Edition");

    DataManager::loadData();

    stack = new QStackedWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stack);

    // 初始化所有页面
    menu = new MenuWidget(this);
    planeSelect = new PlaneSelectWidget(this);
    levelSelect = new LevelSelectWidget(this);
    game = new GameWidget(this);
    highScore = new HighScoreWidget(this);
    shop = new ShopWidget(this);           // 初始化商店
    equipment = new EquipmentWidget(this); // 初始化装备

    // 加入堆栈
    stack->addWidget(menu);        // 0
    stack->addWidget(planeSelect); // 1
    stack->addWidget(levelSelect); // 2
    stack->addWidget(game);        // 3
    stack->addWidget(highScore);   // 4
    stack->addWidget(shop);        // 5 (商店页面)
    stack->addWidget(equipment);   // 6 (装备页面)

    // --- 信号连接 ---

    // 菜单跳转
    connect(menu, &MenuWidget::startClicked, this, [this]()
            { menu->stopMenu(); levelSelect->refreshState(); stack->setCurrentWidget(levelSelect); });
    connect(menu, &MenuWidget::garageClicked, this, [this]()
            { menu->stopMenu(); planeSelect->refreshUI(); stack->setCurrentWidget(planeSelect); });
    connect(menu, &MenuWidget::equipClicked, this, [this]() { // 跳转装备
        menu->stopMenu();
        equipment->refreshUI();
        stack->setCurrentWidget(equipment);
    });
    connect(menu, &MenuWidget::shopClicked, this, [this]() { // 跳转商店
        menu->stopMenu();
        shop->refreshUI();
        stack->setCurrentWidget(shop);
    });
    connect(menu, &MenuWidget::historyClicked, this, [this]()
            {
        highScore->refreshScores(); stack->setCurrentWidget(highScore); });

    // 各子页面返回菜单
    auto backToMenu = [this]()
    {
        stack->setCurrentWidget(menu);
        menu->startMenu();
    };
    connect(planeSelect, &PlaneSelectWidget::backClicked, this, backToMenu);
    connect(levelSelect, &LevelSelectWidget::backClicked, this, backToMenu);
    connect(highScore, &HighScoreWidget::backClicked, this, backToMenu);
    connect(shop, &ShopWidget::backClicked, this, backToMenu);
    connect(equipment, &EquipmentWidget::backClicked, this, backToMenu);

    // 【关键】选择关卡后启动游戏
    connect(levelSelect, &LevelSelectWidget::levelSelected, this, [this](int level)
            {
        game->startGame(level);
        stack->setCurrentWidget(game); });

    // 游戏流程
    connect(game, &GameWidget::gameEnded, this, [this, backToMenu]()
            { game->stopGame(); backToMenu(); });
    connect(game, &GameWidget::levelWon, this, [this]()
            { 
        game->stopGame(); 
        levelSelect->refreshState(); 
        stack->setCurrentWidget(levelSelect); });

    menu->startMenu();
}