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
#include "BossStrategy.h" // 引入新模块

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
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void loadAssets();
    void updateGame();
    void spawnEnemy();
    void spawnBoss();
    void checkCollisions();
    void drawProgressBar(QPainter &p);
    void drawLaserUI(QPainter &p);
    void fireLaser();
    void gameOver();
    void victory();
    void cleanUp();

    // 资源
    QImage imgHero, imgEnemy1, imgEnemy2, imgEnemy3, imgBg;
    QImage imgLaserIcon;

    // 音频
    QSoundEffect *shootSfx;
    QSoundEffect *explodeSfx;
    QSoundEffect *laserSfx;
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

    // --- 新模块对象 ---
    BossStrategy bossStrategy;

    // 大招变量
    bool isLaserActive;
    int laserDurationTimer;
    int laserCooldownTimer;
    const int LASER_COOLDOWN_MAX = 500;
    const int LASER_DURATION_MAX = 20;
};

#endif // GAMEWIDGET_H