#include "LevelSelectWidget.h"
#include "LevelManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>

LevelSelectWidget::LevelSelectWidget(QWidget *parent) : QWidget(parent)
{
    bgImg.load("assets/game_bg.png");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("选择关卡");
    title->setStyleSheet("color: white; font-size: 48px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);
    mainLayout->addSpacing(50);

    QHBoxLayout *levelsLayout = new QHBoxLayout();
    levelsLayout->setSpacing(20);

    for (int i = 1; i <= 5; ++i)
    {
        QPushButton *btn = new QPushButton(QString::number(i));
        btn->setFixedSize(80, 80);
        connect(btn, &QPushButton::clicked, [this, i]()
                { emit levelSelected(i); });
        levelBtns.append(btn);
        levelsLayout->addWidget(btn);
    }
    mainLayout->addLayout(levelsLayout);
    mainLayout->addSpacing(50);

    QPushButton *btnBack = new QPushButton("返回主菜单");
    btnBack->setStyleSheet("QPushButton { background-color: #FF5555; color: white; font-size: 20px; padding: 10px; border-radius: 10px; min-width: 150px; }");
    connect(btnBack, &QPushButton::clicked, this, &LevelSelectWidget::backClicked);
    mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);
}

void LevelSelectWidget::refreshState()
{
    int maxLevel = LevelManager::getMaxUnlockedLevel();
    for (int i = 0; i < 5; ++i)
    {
        if ((i + 1) <= maxLevel)
        {
            levelBtns[i]->setEnabled(true);
            levelBtns[i]->setStyleSheet("QPushButton { background-color: rgba(0, 170, 255, 100); color: white; font-size: 32px; border: 2px solid #00AAFF; border-radius: 40px; } QPushButton:hover { background-color: #00AAFF; }");
        }
        else
        {
            levelBtns[i]->setEnabled(false);
            levelBtns[i]->setStyleSheet("QPushButton { background-color: rgba(50, 50, 50, 150); color: #888; font-size: 32px; border: 2px solid #555; border-radius: 40px; }");
        }
    }
}

void LevelSelectWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (!bgImg.isNull())
        p.drawImage(rect(), bgImg);
    else
        p.fillRect(rect(), Qt::black);
}