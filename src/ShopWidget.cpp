#include "ShopWidget.h"
#include "DataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

ShopWidget::ShopWidget(QWidget *parent) : QWidget(parent)
{
    bgImg.load("assets/game_bg.png");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(50, 20, 50, 20);

    // 1. 顶部：标题 + 金币
    QHBoxLayout *topLayout = new QHBoxLayout();
    QLabel *title = new QLabel("星际补给站 SHOP");
    title->setStyleSheet("color: white; font-size: 40px; font-weight: bold; font-family: 'Microsoft YaHei';");
    coinLabel = new QLabel("Coins: 0");
    coinLabel->setStyleSheet("color: #FFD700; font-size: 30px; font-weight: bold;");
    topLayout->addWidget(title);
    topLayout->addStretch();
    topLayout->addWidget(coinLabel);
    mainLayout->addLayout(topLayout);

    mainLayout->addSpacing(30);

    // 2. 商品列表 (滚动区域)
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame); // 无边框
    scroll->setFixedHeight(300);            // 固定高度

    // 滚动条样式
    scroll->setStyleSheet(R"(
        QScrollArea { background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QScrollBar:vertical { border: none; background: rgba(0,0,0,100); width: 12px; margin: 10px 0; border-radius: 6px; }
        QScrollBar::handle:vertical { background: #00AAFF; min-height: 20px; border-radius: 6px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");

    shopContainer = new QWidget;
    shopContainer->setStyleSheet(".QWidget { background-color: rgba(0, 0, 0, 160); border-radius: 20px; }");
    shopLayout = new QVBoxLayout(shopContainer);
    shopLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    shopLayout->setContentsMargins(40, 30, 40, 30);
    shopLayout->setSpacing(15);

    scroll->setWidget(shopContainer);
    mainLayout->addWidget(scroll);

    mainLayout->addSpacing(20);

    // 3. 底部按钮
    QHBoxLayout *actionLayout = new QHBoxLayout();

    QPushButton *btnBack = new QPushButton("返回主菜单");
    btnBack->setFixedSize(180, 60);
    btnBack->setStyleSheet("QPushButton { background-color: #444; color: white; font-size: 20px; border-radius: 10px; } QPushButton:hover { background-color: #666; }");
    connect(btnBack, &QPushButton::clicked, this, &ShopWidget::backClicked);

    // 购买按钮 (文本动态变化)
    buyBtn = new QPushButton("操作");
    buyBtn->setFixedSize(220, 60);
    buyBtn->setStyleSheet("font-size: 24px; font-weight: bold; border-radius: 10px;");
    buyBtn->setCursor(Qt::PointingHandCursor);

    connect(buyBtn, &QPushButton::clicked, this, [this]()
            {
                // 简化处理：假设购买的是一个固定ID的装备
                int equipIdToBuy = 101; // 假设商店只卖一个基础核心
                Equipment item = DataManager::getEquipmentById(equipIdToBuy);

                if (DataManager::spendCoins(item.cost))
                {
                    DataManager::addEquipment(equipIdToBuy);
                    QMessageBox::information(this, "成功", "购买成功！");
                }
                else
                {
                    QMessageBox::warning(this, "失败", "金币不足！");
                }
                refreshUI(); // 刷新金币和按钮状态
            });

    actionLayout->addWidget(btnBack);
    actionLayout->addStretch();
    actionLayout->addWidget(buyBtn);
    mainLayout->addLayout(actionLayout);

    refreshUI();
}

void ShopWidget::refreshUI()
{
    DataManager::loadData();
    coinLabel->setText("Coins: " + QString::number(DataManager::getCoins()));

    // --- 刷新商品列表 ---
    // （需要根据DataManager::getAvailableShopItems() 来动态生成按钮）
    // 暂时先只处理按钮文本和状态
    // ...
}

void ShopWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (!bgImg.isNull())
        p.drawImage(rect(), bgImg);
    else
        p.fillRect(rect(), Qt::black);
}