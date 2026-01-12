#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QTimer>
#include <QList>
#include <QImage>
#include <QMovie> // 用于播放 GIF Boss
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
    // 输入事件重写
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    // 内部逻辑函数
    void loadAssets();
    void updateGame();
    void spawnEnemy();
    void spawnBoss();
    void checkCollisions();
    void drawProgressBar(QPainter &p);
    void drawUltUI(QPainter &p);
    void fireUlt();
    void gameOver();
    void victory();
    void cleanUp();

    // --- 分辨率适配辅助 ---
    const int LOGICAL_WIDTH = 960;
    const int LOGICAL_HEIGHT = 600;
    void getScaleOffset(double &scale, double &offsetX, double &offsetY);
    QPointF mapToGame(const QPoint &pos);

    // --- 资源变量 ---
    QImage imgHero, imgEnemy1, imgEnemy2, imgEnemy3, imgBg;
    QImage imgUltIcon;

    // --- 音频组件 ---
    QSoundEffect *shootSfx;
    QSoundEffect *explodeSfx;
    QSoundEffect *ultSfx; // 技能音效
    QMediaPlayer *bgmPlayer;
    QAudioOutput *bgmOutput;

    // --- BOSS 动画组件 ---
    QMovie *bossMovie; // 替代视频播放器

    QTimer *gameTimer;

    // --- 游戏基础状态 ---
    double heroX, heroY;
    int heroHp, score;
    bool isGameOver;
    bool isVictory;
    QList<Bullet> bullets;
    QList<Enemy> enemies;

    // --- 关卡逻辑变量 ---
    int heroShootTimer;
    LevelConfig currentLevelConfig;
    int progressCounter;
    bool bossSpawned;
    int enemySpawnTimer;

    // --- 策略模块 ---
    BossStrategy bossStrategy;

    // --- 战机与技能系统 ---
    int currentPlaneId;

    // 大招(Ult)状态
    bool isUltActive;
    int ultDurationTimer;
    int ultCooldownTimer;
    int ULT_COOLDOWN_MAX;

    // 护盾状态 (Plane 3)
    bool isShieldActive;
    int shieldTimer;

    // 时空冻结状态 (Plane 4)
    bool isTimeFrozen;
    int freezeTimer;

    // 核弹全屏闪白透明度 (Plane 2)
    int nukeFlashOpacity;
};

#endif // GAMEWIDGET_H