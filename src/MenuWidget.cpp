#include "MenuWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QPainter>
#include <QFileInfo>
#include <QUrl>

MenuWidget::MenuWidget(QWidget *parent) : QWidget(parent)
{
    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);

    videoSink = new QVideoSink(this);
    player->setVideoOutput(videoSink);

    connect(videoSink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame)
            {
        currentVideoFrame = frame;
        update(); });

    if (QFileInfo::exists("assets/menu_bg.mp4"))
    {
        player->setSource(QUrl::fromLocalFile("assets/menu_bg.mp4"));
        player->setLoops(QMediaPlayer::Infinite);
        audioOutput->setVolume(0);
    }

    menuBgmPlayer = new QMediaPlayer(this);
    menuBgmOutput = new QAudioOutput(this);
    menuBgmPlayer->setAudioOutput(menuBgmOutput);
    if (QFileInfo::exists("assets/menu_bgm.mp3"))
    {
        menuBgmPlayer->setSource(QUrl::fromLocalFile("assets/menu_bgm.mp3"));
        menuBgmPlayer->setLoops(QMediaPlayer::Infinite);
        menuBgmOutput->setVolume(0.5);
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch();

    QLabel *title = new QLabel("星际大战");
    title->setStyleSheet("QLabel { color: #00FFFF; font-size: 64px; font-weight: bold; background: transparent; }");
    title->setAlignment(Qt::AlignCenter);
    title->setGraphicsEffect(new QGraphicsDropShadowEffect(this));
    mainLayout->addWidget(title);

    mainLayout->addSpacing(60);

    QString btnStyle = R"(
        QPushButton {
            background-color: rgba(0, 0, 0, 180); color: white; font-size: 24px;
            border: 2px solid #00AAFF; border-radius: 15px; padding: 12px; min-width: 240px;
        }
        QPushButton:hover { background-color: rgba(0, 170, 255, 150); border-color: white; }
    )";

    QPushButton *btnStart = new QPushButton("开始游戏");
    btnStart->setStyleSheet(btnStyle);
    btnStart->setCursor(Qt::PointingHandCursor);
    connect(btnStart, &QPushButton::clicked, this, &MenuWidget::startClicked);

    // 【新增】战机中心按钮
    QPushButton *btnGarage = new QPushButton("战机中心");
    btnGarage->setStyleSheet(btnStyle);
    btnGarage->setCursor(Qt::PointingHandCursor);
    connect(btnGarage, &QPushButton::clicked, this, &MenuWidget::garageClicked);

    QPushButton *btnHistory = new QPushButton("历史记录");
    btnHistory->setStyleSheet(btnStyle);
    btnHistory->setCursor(Qt::PointingHandCursor);
    connect(btnHistory, &QPushButton::clicked, this, &MenuWidget::historyClicked);

    QPushButton *btnExit = new QPushButton("退出游戏");
    btnExit->setStyleSheet(btnStyle);
    btnExit->setCursor(Qt::PointingHandCursor);
    connect(btnExit, &QPushButton::clicked, qApp, &QApplication::quit);

    QVBoxLayout *btnLayout = new QVBoxLayout();
    btnLayout->setAlignment(Qt::AlignCenter);
    btnLayout->addWidget(btnStart);
    btnLayout->addWidget(btnGarage); // 加入布局
    btnLayout->addWidget(btnHistory);
    btnLayout->addWidget(btnExit);

    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();
}

void MenuWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (currentVideoFrame.isValid())
        p.drawImage(rect(), currentVideoFrame.toImage());
    else
        p.fillRect(rect(), Qt::black);
}

void MenuWidget::startMenu()
{
    player->play();
    if (menuBgmPlayer->source().isValid())
        menuBgmPlayer->play();
}

void MenuWidget::stopMenu()
{
    player->pause();
    menuBgmPlayer->stop();
}