#include "MenuWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QPainter>
#include <QFileInfo>
#include <QUrl>
#include <QMovie> // 确保包含 QMovie

MenuWidget::MenuWidget(QWidget *parent) : QWidget(parent)
{
    // --- 视频背景 ---
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

    // --- 菜单背景音乐 ---
    menuBgmPlayer = new QMediaPlayer(this);
    menuBgmOutput = new QAudioOutput(this);
    menuBgmPlayer->setAudioOutput(menuBgmOutput);
    if (QFileInfo::exists("assets/menu_bgm.mp3"))
    {
        menuBgmPlayer->setSource(QUrl::fromLocalFile("assets/menu_bgm.mp3"));
        menuBgmPlayer->setLoops(QMediaPlayer::Infinite);
        menuBgmOutput->setVolume(0.5);
    }

    // --- UI 布局 ---
    QVBoxLayout *mainVLayout = new QVBoxLayout(this);
    mainVLayout->setContentsMargins(0, 0, 0, 0);
    mainVLayout->setSpacing(0);

    // 添加顶部留白
    mainVLayout->addStretch(1);

    // 中央内容面板
    QWidget *centerPanel = new QWidget;
    centerPanel->setStyleSheet("background-color: rgba(0, 0, 0, 140); border-radius: 20px;");
    centerPanel->setMaximumWidth(700);

    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(50, 60, 50, 60);
    centerLayout->setSpacing(15);

    // 标题
    QLabel *title = new QLabel("⭐ 星际战机 ⭐\nULTIMATE EDITION");
    title->setStyleSheet("color: #00FFFF; font-size: 52px; font-weight: bold; font-family: 'Microsoft YaHei';");
    title->setAlignment(Qt::AlignCenter);
    // 标题阴影
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(Qt::cyan);
    shadow->setOffset(0, 0);
    title->setGraphicsEffect(shadow);
    centerLayout->addWidget(title);
    centerLayout->addSpacing(20);

    // 副标题
    QLabel *subtitle = new QLabel("准备好征服太空了吗？");
    subtitle->setStyleSheet("color: #AAAAFF; font-size: 18px; font-family: 'Microsoft YaHei';");
    subtitle->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(subtitle);
    centerLayout->addSpacing(30);

    // 按钮样式 - 更现代的设计
    QString btnStyle = R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0, 170, 255, 80), stop:1 rgba(0, 100, 200, 100));
            color: white; 
            font-size: 20px; 
            font-weight: bold;
            border: 2px solid #00AAFF; 
            border-radius: 15px; 
            padding: 18px;
            min-height: 65px;
            margin: 10px 0px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0, 200, 255, 150), stop:1 rgba(0, 150, 255, 180));
            border-color: #00FFFF;
            color: #FFFFFF;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0, 100, 200, 200), stop:1 rgba(0, 50, 150, 200));
        }
    )";

    // 辅助函数创建按钮，统一设置样式和连接信号
    auto createBtn = [&](QString text, auto receiver, auto slot)
    {
        QPushButton *btn = new QPushButton(text);
        btn->setStyleSheet(btnStyle);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, receiver, slot);
        centerLayout->addWidget(btn);
    };

    // 添加菜单项
    createBtn("🚀  开始任务  MISSION", this, &MenuWidget::startClicked);
    createBtn("🛸  机库中心  HANGAR", this, &MenuWidget::garageClicked);
    createBtn("🛡️  装备配置  EQUIP", this, &MenuWidget::equipClicked);
    createBtn("🛒  补给商店  SHOP", this, &MenuWidget::shopClicked);
    createBtn("🏆  荣誉殿堂  RANK", this, &MenuWidget::historyClicked);

    centerLayout->addSpacing(20);

    createBtn("❌  退出游戏  EXIT", qApp, &QApplication::quit);

    // 将中央面板添加到主布局
    QHBoxLayout *mainHLayout = new QHBoxLayout;
    mainHLayout->setContentsMargins(0, 0, 0, 0);
    mainHLayout->addStretch();
    mainHLayout->addWidget(centerPanel);
    mainHLayout->addStretch();

    centerLayout->addStretch();

    mainVLayout->addLayout(mainHLayout, 1);
    mainVLayout->addStretch(1);
}

void MenuWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (currentVideoFrame.isValid())
    {
        p.drawImage(rect(), currentVideoFrame.toImage());
    }
    else
    {
        p.fillRect(rect(), Qt::black); // 视频加载失败则黑屏
    }
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
    if (menuBgmPlayer->source().isValid())
        menuBgmPlayer->stop();
}