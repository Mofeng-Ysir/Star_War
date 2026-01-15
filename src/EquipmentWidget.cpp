#include "EquipmentWidget.h"
#include "DataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QDebug>

EquipmentWidget::EquipmentWidget(QWidget *parent) : QWidget(parent)
{
    bgImg.load("assets/game_bg.png");
    currentSelectedSlotType = 0;

    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // --- 左侧：角色装备栏 ---
    QWidget *leftPanel = new QWidget;
    leftPanel->setFixedWidth(300);
    leftPanel->setStyleSheet("background-color: rgba(0,0,0,150); border-right: 2px solid #555;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    QLabel *title = new QLabel("装备 LOADOUT");
    title->setStyleSheet("color: white; font-size: 28px; font-weight: bold; margin-bottom: 20px;");
    title->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(title);

    // 创建三个插槽
    auto createSlot = [&](const QString &name, int type, QPushButton *&btn, QLabel *&lbl)
    {
        QLabel *header = new QLabel(name);
        header->setStyleSheet("color: #AAA; font-size: 18px; margin-top: 20px;");
        leftLayout->addWidget(header);

        btn = new QPushButton("Empty");
        btn->setFixedSize(260, 80);
        connect(btn, &QPushButton::clicked, [this, type]()
                { onSlotClicked(type); });
        leftLayout->addWidget(btn);

        lbl = new QLabel("");
        lbl->setStyleSheet("color: #888; font-size: 14px;");
        lbl->setWordWrap(true);
        leftLayout->addWidget(lbl);
    };

    createSlot("核心 (CORE)", TYPE_CORE, btnCore, lblCore);
    createSlot("装甲 (ARMOR)", TYPE_ARMOR, btnArmor, lblArmor);
    createSlot("引擎 (ENGINE)", TYPE_ENGINE, btnEngine, lblEngine);

    leftLayout->addStretch();
    QPushButton *backBtn = new QPushButton("返回主菜单");
    backBtn->setStyleSheet("background-color: #555; color: white; padding: 10px; font-size: 18px;");
    connect(backBtn, &QPushButton::clicked, this, &EquipmentWidget::backClicked);
    leftLayout->addWidget(backBtn);

    // --- 右侧：仓库列表 ---
    QWidget *rightPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    QLabel *invTitle = new QLabel("仓库 INVENTORY (点击穿戴)");
    invTitle->setStyleSheet("color: gold; font-size: 24px; font-weight: bold;");
    rightLayout->addWidget(invTitle);

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("background: transparent; border: none;");

    inventoryContainer = new QWidget;
    inventoryLayout = new QVBoxLayout(inventoryContainer);
    inventoryLayout->setAlignment(Qt::AlignTop);
    scroll->setWidget(inventoryContainer);
    rightLayout->addWidget(scroll);

    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(rightPanel);

    refreshUI();
}

void EquipmentWidget::updateSlotUI(int type, QPushButton *btn, QLabel *lbl)
{
    int id = DataManager::getEquippedId((EquipType)type);
    if (id == -1)
    {
        btn->setText("未装备");
        btn->setStyleSheet("background-color: #333; color: #888; border: 2px dashed #555; border-radius: 10px;");
        lbl->setText("");
    }
    else
    {
        Equipment e = DataManager::getEquipmentById(id);
        QString color = (e.tier == TIER_EPIC) ? "#A020F0" : (e.tier == TIER_RARE ? "#00BFFF" : "#FFFFFF");
        btn->setText(e.name);
        btn->setStyleSheet(QString("background-color: #222; color: %1; border: 2px solid %1; border-radius: 10px; font-size: 20px; font-weight: bold;").arg(color));
        lbl->setText(e.desc);
    }
}

void EquipmentWidget::refreshUI()
{
    updateSlotUI(TYPE_CORE, btnCore, lblCore);
    updateSlotUI(TYPE_ARMOR, btnArmor, lblArmor);
    updateSlotUI(TYPE_ENGINE, btnEngine, lblEngine);

    onSlotClicked(currentSelectedSlotType); // 刷新右侧列表
}

void EquipmentWidget::onSlotClicked(int type)
{
    currentSelectedSlotType = type;

    // 清空列表
    QLayoutItem *item;
    while ((item = inventoryLayout->takeAt(0)) != nullptr)
    {
        delete item->widget();
        delete item;
    }

    // 填充符合当前部位的装备
    QList<int> inv = DataManager::getInventory();
    bool found = false;
    for (int id : inv)
    {
        Equipment e = DataManager::getEquipmentById(id);
        if ((int)e.type == type)
        {
            found = true;
            QPushButton *itemBtn = new QPushButton();
            itemBtn->setFixedHeight(80);

            QString color = (e.tier == TIER_EPIC) ? "#A020F0" : (e.tier == TIER_RARE ? "#00BFFF" : "#FFFFFF");
            QString border = (DataManager::getEquippedId((EquipType)type) == id) ? "border: 4px solid #00FF00;" : "border: 1px solid #555;";

            itemBtn->setText(QString("%1\n%2").arg(e.name).arg(e.desc));
            itemBtn->setStyleSheet(QString("text-align: left; padding: 10px; background-color: rgba(0,0,0,100); color: %1; font-size: 18px; border-radius: 5px; %2").arg(color).arg(border));

            connect(itemBtn, &QPushButton::clicked, [this, id]()
                    { onInventoryItemClicked(id); });
            inventoryLayout->addWidget(itemBtn);
        }
    }
    if (!found)
    {
        QLabel *emptyLabel = new QLabel("暂无该部位装备");
        emptyLabel->setStyleSheet("color: #888; font-size: 20px; margin: 20px;");
        inventoryLayout->addWidget(emptyLabel);
    }
}

void EquipmentWidget::onInventoryItemClicked(int id)
{
    Equipment e = DataManager::getEquipmentById(id);
    DataManager::equipItem(e.type, id);
    refreshUI();
}

void EquipmentWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (!bgImg.isNull())
        p.drawImage(rect(), bgImg);
    else
        p.fillRect(rect(), Qt::black);
}