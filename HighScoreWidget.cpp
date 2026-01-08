#include "HighScoreWidget.h"
#include "ScoreManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QScrollBar>

HighScoreWidget::HighScoreWidget(QWidget *parent) : QWidget(parent)
{
    bgImg.load("assets/game_bg.png");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(50, 40, 50, 40);

    QLabel *title = new QLabel("Top 10 英雄榜");
    title->setStyleSheet("color: #FFD700; font-size: 60px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    title->setGraphicsEffect(new QGraphicsDropShadowEffect(this));
    mainLayout->addWidget(title);
    mainLayout->addSpacing(20);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(R"(
        QScrollArea { background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QScrollBar:vertical { border: none; background: rgba(0,0,0,100); width: 12px; border-radius: 6px; }
        QScrollBar::handle:vertical { background: #00AAFF; min-height: 20px; border-radius: 6px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");

    scrollContent = new QWidget();
    scrollContent->setStyleSheet(".QWidget { background-color: rgba(0, 0, 0, 160); border-radius: 20px; }");

    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    contentLayout->setContentsMargins(40, 30, 40, 30);
    contentLayout->setSpacing(15);

    for (int i = 0; i < 10; i++)
    {
        QLabel *lbl = new QLabel();
        lbl->setAlignment(Qt::AlignCenter);
        scoreLabels.append(lbl);
        contentLayout->addWidget(lbl);
    }

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);
    mainLayout->addSpacing(20);

    QPushButton *btnBack = new QPushButton("返回主菜单");
    btnBack->setStyleSheet("QPushButton { background-color: #00AAFF; color: white; font-size: 24px; font-weight: bold; border-radius: 12px; padding: 15px; min-width: 200px; } QPushButton:hover { background-color: #0088CC; }");
    btnBack->setCursor(Qt::PointingHandCursor);
    connect(btnBack, &QPushButton::clicked, this, &HighScoreWidget::backClicked);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(btnBack);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

void HighScoreWidget::refreshScores()
{
    QList<int> scores = ScoreManager::loadScores();
    if (scores.isEmpty())
    {
        scoreLabels[0]->setText("暂无记录");
        scoreLabels[0]->setStyleSheet("color: #AAAAAA; font-size: 36px; padding: 20px; font-weight: bold;");
        scoreLabels[0]->setVisible(true);
        for (int i = 1; i < 10; i++)
            scoreLabels[i]->setVisible(false);
        return;
    }

    for (int i = 0; i < 10; i++)
    {
        if (i < scores.size())
        {
            QString rankStr;
            QString style = "font-family: 'Consolas'; font-weight: bold; font-size: 36px; padding: 5px; color: white;";
            if (i == 0)
            {
                rankStr = "👑 1ST  ";
                style = "color: #FFD700; font-size: 42px; font-weight: bold; font-family: 'Consolas';";
            }
            else if (i == 1)
            {
                rankStr = "🥈 2ND  ";
                style = "color: #E0E0E0; font-size: 38px; font-weight: bold; font-family: 'Consolas';";
            }
            else if (i == 2)
            {
                rankStr = "🥉 3RD  ";
                style = "color: #CD7F32; font-size: 38px; font-weight: bold; font-family: 'Consolas';";
            }
            else
            {
                rankStr = QString("%1.  ").arg(i + 1, -2);
            }

            scoreLabels[i]->setText(rankStr + QString::number(scores[i]));
            scoreLabels[i]->setStyleSheet(style);
            scoreLabels[i]->setVisible(true);
        }
        else
        {
            scoreLabels[i]->setVisible(false);
        }
    }
}

void HighScoreWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (!bgImg.isNull())
    {
        p.drawImage(rect(), bgImg);
        p.fillRect(rect(), QColor(0, 0, 0, 50));
    }
    else
    {
        p.fillRect(rect(), QColor(20, 20, 40));
    }
}