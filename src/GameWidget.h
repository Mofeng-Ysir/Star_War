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
#include "BossStrategy.h"

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
    void drawUltUI(QPainter &p); // 改名为 Ult (大招)
    void fireUlt();              // 改名为 fireUlt
    void gameOver();
    void victory();
    void cleanUp();

    // 资源
    QImage imgHero, imgEnemy1, imgEnemy2, imgEnemy3, imgBg;
    QImage imgUltIcon; // 大招图标

    // 音频
    QSoundEffect *shootSfx;
    QSoundEffect *explodeSfx;
    QSoundEffect *ultSfx; // 大招音效
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

    // 策略模块
    BossStrategy bossStrategy;

    // --- 战机与技能系统 ---
    int currentPlaneId; // 当前战机ID

    // 技能状态
    bool isUltActive;     // 大招持续中 (激光/弹幕等)
    int ultDurationTimer; // 持续时间计时
    int ultCooldownTimer; // 冷却计时
    int ULT_COOLDOWN_MAX; // 最大冷却 (根据飞机不同可变)

    // 特殊状态
    bool isShieldActive; // 护盾 (幻影号)
    int shieldTimer;

    bool isTimeFrozen; // 时空冻结 (虚空号)
    int freezeTimer;
};

#endif // GAMEWIDGET_H