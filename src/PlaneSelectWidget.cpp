#include "PlaneSelectWidget.h"
#include "DataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QScrollArea> // 新增
#include <QScrollBar>  // 新增

PlaneSelectWidget::PlaneSelectWidget(QWidget *parent) : QWidget(parent)
{
    bgImg.load("assets/game_bg.png");
    selectedPreviewId = 0;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(50, 20, 50, 20);

    // 1. 顶部：标题 + 金币
    QHBoxLayout *topLayout = new QHBoxLayout();
    QLabel *title = new QLabel("机库 HANGAR");
    title->setStyleSheet("color: white; font-size: 40px; font-weight: bold; font-family: 'Microsoft YaHei';");
    coinLabel = new QLabel("Coins: 0");
    coinLabel->setStyleSheet("color: #FFD700; font-size: 30px; font-weight: bold;");
    topLayout->addWidget(title);
    topLayout->addStretch();
    topLayout->addWidget(coinLabel);
    mainLayout->addLayout(topLayout);

    mainLayout->addSpacing(30);

    // 2. 中间：飞机列表 (图标按钮)
    QHBoxLayout *planesLayout = new QHBoxLayout();
    planesLayout->setSpacing(20);

    for (int i = 0; i < 5; ++i)
    {
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(100, 100);
        btn->setStyleSheet("background-color: rgba(0,0,0,150); border: 2px solid #555; border-radius: 10px;");

        QString imgPath = QString("assets/plane%1.png").arg(i);
        if (i == 0)
            imgPath = "assets/hero.png";

        if (!QFileInfo::exists(imgPath))
        {
            imgPath = "assets/hero.png";
        }

        btn->setIcon(QIcon(imgPath));
        btn->setIconSize(QSize(80, 80));
        btn->setCursor(Qt::PointingHandCursor);

        connect(btn, &QPushButton::clicked, [this, i]()
                { onPlaneClicked(i); });
        planeBtns.append(btn);
        planesLayout->addWidget(btn);
    }
    mainLayout->addLayout(planesLayout);
    mainLayout->addSpacing(30);

    // 3. 下方：详情信息框 (带滚动条) --- 【核心修改区域】 ---

    QScrollArea *infoScroll = new QScrollArea(this);
    infoScroll->setWidgetResizable(true); // 让内部的 label 自动撑满宽度
    infoScroll->setFixedHeight(220);      // 固定滚动区域的高度

    // 设置滚动区域样式 (边框、背景、滚动条美化)
    infoScroll->setStyleSheet(R"(
        QScrollArea {
            border: 2px solid #00AAFF;
            background-color: rgba(0, 20, 40, 200);
            border-radius: 15px;
        }
        /* 隐藏边框线 */
        QScrollArea > QWidget > QWidget { background: transparent; }
        
        /* 垂直滚动条美化 */
        QScrollBar:vertical {
            border: none;
            background: rgba(0,0,0,50);
            width: 12px;
            margin: 10px 0 10px 0;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background: #00AAFF;
            min-height: 20px;
            border-radius: 6px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");

    // 初始化 infoLabel
    infoLabel = new QLabel("详情加载中...");
    infoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    infoLabel->setWordWrap(true); // 必须开启自动换行

    // label 本身背景透明，字体样式在这里设置
    infoLabel->setStyleSheet("color: #EEE; font-size: 22px; padding: 15px; background: transparent; border: none;");

    // 将 label 放入滚动区域
    infoScroll->setWidget(infoLabel);

    // 将滚动区域加入布局
    mainLayout->addWidget(infoScroll);

    // ----------------------------------------------------

    mainLayout->addSpacing(20);

    // 4. 底部：动作按钮区域
    QHBoxLayout *actionLayout = new QHBoxLayout();

    QPushButton *btnBack = new QPushButton("返回主菜单");
    btnBack->setFixedSize(180, 60);
    btnBack->setStyleSheet(R"(
        QPushButton { background-color: #444; color: white; font-size: 20px; border-radius: 10px; }
        QPushButton:hover { background-color: #666; }
    )");
    connect(btnBack, &QPushButton::clicked, this, &PlaneSelectWidget::backClicked);

    actionBtn = new QPushButton("操作");
    actionBtn->setFixedSize(220, 60);
    actionBtn->setStyleSheet("font-size: 24px; font-weight: bold; border-radius: 10px;");
    actionBtn->setCursor(Qt::PointingHandCursor);

    connect(actionBtn, &QPushButton::clicked, [this]()
            {
        if (DataManager::isPlaneUnlocked(selectedPreviewId)) {
            DataManager::setCurrentPlane(selectedPreviewId);
            refreshUI(); 
        } else {
            int cost = DataManager::getPlaneStats(selectedPreviewId).cost;
            if (DataManager::spendCoins(cost)) {
                DataManager::unlockPlane(selectedPreviewId);
                DataManager::setCurrentPlane(selectedPreviewId);
                QMessageBox::information(this, "交易成功", "恭喜！新战机已入库！");
                refreshUI();
            } else {
                QMessageBox::warning(this, "资金不足", "击败更多敌人来获取战利品吧！");
            }
        } });

    actionLayout->addWidget(btnBack);
    actionLayout->addStretch();
    actionLayout->addWidget(actionBtn);
    mainLayout->addLayout(actionLayout);

    refreshUI();
}

void PlaneSelectWidget::refreshUI()
{
    DataManager::loadData();
    coinLabel->setText(QString("战利品: %1").arg(DataManager::getCoins()));

    int currentId = DataManager::getCurrentPlaneId();

    for (int i = 0; i < 5; ++i)
    {
        bool unlocked = DataManager::isPlaneUnlocked(i);
        QString style = "border-radius: 10px; ";

        if (i == currentId)
        {
            style += "background-color: rgba(0, 255, 0, 50); border: 4px solid #00FF00;";
        }
        else if (i == selectedPreviewId)
        {
            style += "background-color: rgba(255, 255, 255, 30); border: 4px solid #FFFFFF;";
        }
        else if (unlocked)
        {
            style += "background-color: rgba(0, 0, 0, 150); border: 2px solid #888;";
        }
        else
        {
            style += "background-color: rgba(50, 0, 0, 200); border: 2px solid #500;";
        }
        planeBtns[i]->setStyleSheet(style);
    }

    PlaneStats s = DataManager::getPlaneStats(selectedPreviewId);
    bool isUnlocked = DataManager::isPlaneUnlocked(selectedPreviewId);

    QString statusText;
    if (isUnlocked)
    {
        statusText = (selectedPreviewId == currentId)
                         ? "<span style='color:#00FF00; font-weight:bold;'>[ 当前出击 ]</span>"
                         : "<span style='color:#AAAAAA;'>[ 待机中 ]</span>";
    }
    else
    {
        statusText = "<span style='color:#FF4444; font-weight:bold;'>[ 未解锁 ]</span>";
    }

    QString html = QString(
                       "<div>"
                       "   <span style='font-size:32px; font-weight:bold; color:#FFF;'>%1</span> &nbsp;&nbsp; %2<br>"
                       "   <span style='color:#888;'>----------------------------------------</span><br>"
                       "   <span style='font-size:24px;'>售价: <span style='color:#FFD700;'>%3</span> 战利品</span><br>"
                       "   生命值: <span style='color:#00FF00;'>%4</span> &nbsp;|&nbsp; 机动性: <span style='color:#00FFFF;'>%5</span><br><br>"
                       "   <span style='font-size:20px; line-height:150%;'>%6</span>"
                       "</div>")
                       .arg(s.name)
                       .arg(statusText)
                       .arg(s.cost)
                       .arg(s.hp)
                       .arg(s.speed)
                       .arg(s.desc.replace("\n", "<br>"));

    infoLabel->setText(html);

    if (isUnlocked)
    {
        if (selectedPreviewId == currentId)
        {
            actionBtn->setText("已装备");
            actionBtn->setEnabled(false);
            actionBtn->setStyleSheet("background-color: #333; color: #888; border: 2px solid #555; border-radius: 10px;");
        }
        else
        {
            actionBtn->setText("出击");
            actionBtn->setEnabled(true);
            actionBtn->setStyleSheet("background-color: #00AA00; color: white; border: 2px solid #00FF00; border-radius: 10px;");
        }
    }
    else
    {
        actionBtn->setText("购买 (" + QString::number(s.cost) + ")");
        actionBtn->setEnabled(true);
        if (DataManager::getCoins() >= s.cost)
        {
            actionBtn->setStyleSheet("background-color: #DDCC00; color: black; border: 2px solid #FFF; border-radius: 10px;");
        }
        else
        {
            actionBtn->setStyleSheet("background-color: #550000; color: #AAA; border: 2px solid #500; border-radius: 10px;");
        }
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