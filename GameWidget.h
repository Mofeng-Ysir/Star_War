#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QTimer>
#include <QList>
#include <QImage>
#include <QVideoSink>
#include <QVideoFrame>
#include "common.h"

class GameWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);
    void startGame(int level);
    void stopGame();

signals:
    void gameEnded();
    void levelWon();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;   // 点击释放大招
    void mouseReleaseEvent(QMouseEvent *event) override; // 【关键修复】补上了这个声明
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void loadAssets();
    void updateGame();
    void spawnEnemy();
    void spawnBoss();
    void checkCollisions();
    void drawProgressBar(QPainter &p);
    void drawLaserUI(QPainter &p); // 绘制大招UI
    void fireLaser();              // 释放大招逻辑
    void gameOver();
    void victory();
    void cleanUp();

    // 资源
    QImage imgHero, imgEnemy1, imgEnemy2, imgEnemy3, imgBg;
    QImage imgLaserIcon; // 大招图标

    // 音频
    QSoundEffect *shootSfx;
    QSoundEffect *explodeSfx;
    QSoundEffect *laserSfx; // 激光音效
    QMediaPlayer *bgmPlayer;
    QAudioOutput *bgmOutput;

    // BOSS 视频
    QMediaPlayer *bossVideoPlayer;
    QVideoSink *bossVideoSink;
    QVideoFrame currentBossFrame;

    QTimer *gameTimer;

    // 游戏状态
    double heroX, heroY;
    int heroHp, score;
    bool isGameOver;
    bool isVictory;
    QList<Bullet> bullets;
    QList<Enemy> enemies;

    int heroShootTimer;
    LevelConfig currentLevelConfig;
    int progressCounter;
    bool bossSpawned;
    int enemySpawnTimer;

    // Boss AI 变量
    double bossTime;
    double bossAttackAngle; // 用于螺旋弹幕等计算

    // --- 大招系统变量 ---
    bool isLaserActive;                 // 激光是否正在喷射
    int laserDurationTimer;             // 激光持续时间计时器
    int laserCooldownTimer;             // 激光冷却计时器
    const int LASER_COOLDOWN_MAX = 500; // 冷却时间 (约8秒)
    const int LASER_DURATION_MAX = 20;  // 持续时间 (约0.3秒)
};

#endif // GAMEWIDGET_H