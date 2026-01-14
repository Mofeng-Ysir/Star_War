#include "PlaneSelectWidget.h"
#include "DataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QScrollArea>
#include <QScrollBar>

PlaneSelectWidget::PlaneSelectWidget(QWidget *parent) : QWidget(parent)
{
    bgImg.load("assets/game_bg.png");
    selectedPreviewId = 0;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(50, 20, 50, 20);

    // 1. 顶部
    QHBoxLayout *topLayout = new QHBoxLayout();
    QLabel *title = new QLabel("机库 HANGAR");
    title->setStyleSheet("color: white; font-size: 40px; font-weight: bold; font-family: 'Microsoft YaHei';");
    coinLabel = new QLabel("Coins: 0");
    coinLabel->setStyleSheet("color: #FFD700; font-size: 30px; font-weight: bold;");
    topLayout->addWidget(title);
    topLayout->addStretch();
    topLayout->addWidget(coinLabel);
    mainLayout->addLayout(topLayout);

    mainLayout->addSpacing(20);

    // 2. 飞机列表
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
            imgPath = "assets/hero.png";

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

    // 3. 详情框 (带滚动)
    QScrollArea *infoScroll = new QScrollArea(this);
    infoScroll->setWidgetResizable(true);
    infoScroll->setFixedHeight(260); // 稍微加高以容纳进度条

    infoScroll->setStyleSheet(R"(
        QScrollArea { border: 2px solid #00AAFF; background-color: rgba(0, 20, 40, 200); border-radius: 15px; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QScrollBar:vertical { border: none; background: rgba(0,0,0,50); width: 12px; margin: 10px 0; border-radius: 6px; }
        QScrollBar::handle:vertical { background: #00AAFF; min-height: 20px; border-radius: 6px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");

    infoLabel = new QLabel("详情加载中...");
    infoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #EEE; font-size: 20px; padding: 15px; background: transparent; border: none;");

    infoScroll->setWidget(infoLabel);
    mainLayout->addWidget(infoScroll);

    mainLayout->addSpacing(20);

    // 4. 底部按钮
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
            style += "background-color: rgba(0, 255, 0, 50); border: 4px solid #00FF00;";
        else if (i == selectedPreviewId)
            style += "background-color: rgba(255, 255, 255, 30); border: 4px solid #FFFFFF;";
        else if (unlocked)
            style += "background-color: rgba(0, 0, 0, 150); border: 2px solid #888;";
        else
            style += "background-color: rgba(50, 0, 0, 200); border: 2px solid #500;";
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

    // --- 核心修改：生成条形图 HTML ---
    // 辅助 Lambda：生成一个进度条的 HTML 字符串
    // val: 当前值, max: 最大值, color: 颜色
    auto makeBar = [](double val, double max, QString color) -> QString
    {
        int width = (int)((val / max) * 150); // 进度条总长 150px
        if (width > 150)
            width = 150;
        // 使用 table 来模拟进度条，背景深灰，前景彩色
        return QString(
                   "<table width='150' border='0' cellspacing='0' cellpadding='0' bgcolor='#333333'>"
                   "<tr><td width='%1' bgcolor='%2' height='8'></td>"
                   "<td></td></tr></table>")
            .arg(width)
            .arg(color);
    };

    // 定义各项属性的最大值 (用于计算比例)
    const double MAX_ATK = 5.0;
    const double MAX_DEF = 5.0;
    const double MAX_SPD = 3.0; // 射速
    const double MAX_HP = 8.0;

    QString html = QString(
                       "<div>"
                       "   <table width='100%'>"
                       "   <tr>"
                       "       <td colspan='2'><span style='font-size:32px; font-weight:bold; color:#FFF;'>%1</span> &nbsp; %2</td>"
                       "   </tr>"
                       "   <tr><td colspan='2'><hr></td></tr>"

                       // 第一行：攻击 & 防御
                       "   <tr>"
                       "       <td width='50%'>"
                       "           <span style='font-size:18px; color:#AAA;'>攻击火力: </span><span style='font-size:22px; color:#FF4444; font-weight:bold;'>%3</span><br>"
                       "           %4" // 攻击条
                       "       </td>"
                       "       <td width='50%'>"
                       "           <span style='font-size:18px; color:#AAA;'>装甲防御: </span><span style='font-size:22px; color:#4444FF; font-weight:bold;'>%5</span><br>"
                       "           %6" // 防御条
                       "       </td>"
                       "   </tr>"
                       "   <tr><td colspan='2' height='10'></td></tr>" // 空行间距

                       // 第二行：射速 & 耐久
                       "   <tr>"
                       "       <td>"
                       "           <span style='font-size:18px; color:#AAA;'>武器射速: </span><span style='font-size:22px; color:#FFFF00; font-weight:bold;'>%7</span><br>"
                       "           %8" // 射速条
                       "       </td>"
                       "       <td>"
                       "           <span style='font-size:18px; color:#AAA;'>舰体耐久: </span><span style='font-size:22px; color:#00FF00; font-weight:bold;'>%9</span><br>"
                       "           %10" // 耐久条
                       "       </td>"
                       "   </tr>"
                       "   </table>"

                       "<br>"
                       "   <span style='font-size:22px; color:gold;'>售价: %11</span><br>"
                       "   <span style='font-size:18px; color:#DDD;'>%12</span>"
                       "</div>")
                       .arg(s.name)
                       .arg(statusText)
                       .arg(s.viewAtk)
                       .arg(makeBar(s.viewAtk, MAX_ATK, "#FF4444"))
                       .arg(s.viewDef)
                       .arg(makeBar(s.viewDef, MAX_DEF, "#4444FF"))
                       .arg(s.viewRate)
                       .arg(makeBar(s.viewRate, MAX_SPD, "#FFFF00"))
                       .arg(s.viewHp)
                       .arg(makeBar(s.viewHp, MAX_HP, "#00FF00"))
                       .arg(s.cost)
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
        actionBtn->setText("购买");
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