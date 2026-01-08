#include "PlaneSelectWidget.h"
#include "DataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

PlaneSelectWidget::PlaneSelectWidget(QWidget *parent) : QWidget(parent)
{
    bgImg.load("assets/game_bg.png");
    selectedPreviewId = 0;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(50, 20, 50, 20);

    // 顶部：标题 + 金币
    QHBoxLayout *topLayout = new QHBoxLayout();
    QLabel *title = new QLabel("机库 HANGAR");
    title->setStyleSheet("color: white; font-size: 40px; font-weight: bold;");
    coinLabel = new QLabel("Coins: 0");
    coinLabel->setStyleSheet("color: gold; font-size: 30px; font-weight: bold;");
    topLayout->addWidget(title);
    topLayout->addStretch();
    topLayout->addWidget(coinLabel);
    mainLayout->addLayout(topLayout);

    mainLayout->addSpacing(20);

    // 中间：飞机列表 (水平排列)
    QHBoxLayout *planesLayout = new QHBoxLayout();
    planesLayout->setSpacing(15);

    for (int i = 0; i < 5; ++i)
    {
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(120, 120);
        btn->setStyleSheet("background-color: rgba(0,0,0,150); border: 2px solid #555; border-radius: 10px;");

        // --- 修改开始：根据 ID 加载不同的飞机图片 ---
        QString imgPath;
        if (i == 0)
        {
            imgPath = "assets/hero.png"; // 初始机
        }
        else
        {
            // 对应 assets/plane1.png, assets/plane2.png ...
            imgPath = QString("assets/plane%1.png").arg(i);
        }

        // 检查文件是否存在，不存在则用默认图兜底，防止空白
        if (!QFileInfo::exists(imgPath))
        {
            qDebug() << "Warning: Image not found:" << imgPath;
            imgPath = "assets/hero.png";
        }
        // --- 修改结束 ---

        btn->setIcon(QIcon(imgPath));
        btn->setIconSize(QSize(80, 80));

        connect(btn, &QPushButton::clicked, [this, i]()
                { onPlaneClicked(i); });
        planeBtns.append(btn);
        planesLayout->addWidget(btn);
    }

    mainLayout->addLayout(planesLayout);

    // 下方：详情 + 动作按钮
    infoLabel = new QLabel("详情");
    infoLabel->setStyleSheet("color: #DDD; font-size: 20px; border: 1px solid #555; padding: 15px; background: rgba(0,0,0,100); border-radius: 10px;");
    infoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    infoLabel->setFixedHeight(150);
    mainLayout->addWidget(infoLabel);

    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addStretch();

    actionBtn = new QPushButton("操作");
    actionBtn->setFixedSize(200, 60);
    actionBtn->setStyleSheet("font-size: 24px; font-weight: bold; border-radius: 10px;");
    connect(actionBtn, &QPushButton::clicked, [this]()
            {
        if (DataManager::isPlaneUnlocked(selectedPreviewId)) {
            // 装备
            DataManager::setCurrentPlane(selectedPreviewId);
            refreshUI();
        } else {
            // 购买
            int cost = DataManager::getPlaneStats(selectedPreviewId).cost;
            if (DataManager::spendCoins(cost)) {
                DataManager::unlockPlane(selectedPreviewId);
                DataManager::setCurrentPlane(selectedPreviewId); // 买完自动装备
                QMessageBox::information(this, "成功", "购买成功！");
                refreshUI();
            } else {
                QMessageBox::warning(this, "失败", "金币不足！");
            }
        } });
    actionLayout->addWidget(actionBtn);
    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    // 底部：返回
    QPushButton *btnBack = new QPushButton("返回主菜单");
    btnBack->setStyleSheet("background-color: #555; color: white; font-size: 18px; padding: 10px; border-radius: 5px;");
    connect(btnBack, &QPushButton::clicked, this, &PlaneSelectWidget::backClicked);
    mainLayout->addWidget(btnBack, 0, Qt::AlignLeft);
}

void PlaneSelectWidget::refreshUI()
{
    DataManager::loadData();
    coinLabel->setText("Coins: " + QString::number(DataManager::getCoins()));

    int currentId = DataManager::getCurrentPlaneId();

    for (int i = 0; i < 5; ++i)
    {
        bool unlocked = DataManager::isPlaneUnlocked(i);
        QString style = "border-radius: 10px; ";
        if (i == currentId)
        {
            style += "background-color: rgba(0, 255, 0, 50); border: 3px solid #0F0;"; // 当前装备：绿框
        }
        else if (i == selectedPreviewId)
        {
            style += "background-color: rgba(255, 255, 255, 50); border: 3px solid #FFF;"; // 当前选中预览：白框
        }
        else if (unlocked)
        {
            style += "background-color: rgba(0, 0, 0, 150); border: 2px solid #888;"; // 已解锁：灰框
        }
        else
        {
            style += "background-color: rgba(50, 0, 0, 200); border: 2px solid #500;"; // 未解锁：红黑框
        }
        planeBtns[i]->setStyleSheet(style);
    }

    // 更新详情文字
    PlaneStats s = DataManager::getPlaneStats(selectedPreviewId);
    QString status = DataManager::isPlaneUnlocked(selectedPreviewId) ? (selectedPreviewId == currentId ? "<font color='#00FF00'>[已装备]</font>" : "<font color='#AAAAAA'>[已拥有]</font>") : "<font color='#FF0000'>[未解锁]</font>";

    infoLabel->setText(QString(
                           "<b>%1</b> %2<br>"
                           "----------------<br>"
                           "价格: <font color='gold'>%3</font><br>"
                           "生命: %4 | 速度: %5<br><br>"
                           "%6")
                           .arg(s.name)
                           .arg(status)
                           .arg(s.cost)
                           .arg(s.hp)
                           .arg(s.speed)
                           .arg(s.desc));

    // 更新按钮状态
    if (DataManager::isPlaneUnlocked(selectedPreviewId))
    {
        if (selectedPreviewId == currentId)
        {
            actionBtn->setText("已装备");
            actionBtn->setEnabled(false);
            actionBtn->setStyleSheet("background-color: #333; color: #888; border: none;");
        }
        else
        {
            actionBtn->setText("装备");
            actionBtn->setEnabled(true);
            actionBtn->setStyleSheet("background-color: #00AA00; color: white;");
        }
    }
    else
    {
        actionBtn->setText("购买 " + QString::number(s.cost));
        actionBtn->setEnabled(true);
        if (DataManager::getCoins() >= s.cost)
            actionBtn->setStyleSheet("background-color: #DDCC00; color: black;");
        else
            actionBtn->setStyleSheet("background-color: #550000; color: #AAA;");
    }
}

void PlaneSelectWidget::onPlaneClicked(int id)
{
    selectedPreviewId = id;
    refreshUI();
}

void PlaneSelectWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (!bgImg.isNull())
        p.drawImage(rect(), bgImg);
    else
        p.fillRect(rect(), Qt::black);
}