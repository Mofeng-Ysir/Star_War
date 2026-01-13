#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QTimer>
#include <QList>
#include <QImage>
#include <QMovie>
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

    // 资源
    QImage imgHero, imgEnemy1, imgEnemy2, imgEnemy3, imgBg;
    QImage imgUltIcon;

    // 【新增】子弹图片列表
    QList<QImage> bulletImages;

    // 音频
    QSoundEffect *shootSfx;
    QSoundEffect *explodeSfx;
    QSoundEffect *ultSfx;
    QMediaPlayer *bgmPlayer;
    QAudioOutput *bgmOutput;

    // BOSS 动画
    QMovie *bossMovie;

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
    int currentPlaneId;

    // 技能状态
    bool isUltActive;
    int ultDurationTimer;
    int ultCooldownTimer;
    int ULT_COOLDOWN_MAX;

    bool isShieldActive;
    int shieldTimer;

    bool isTimeFrozen;
    int freezeTimer;

    // 核弹全屏闪白透明度
    int nukeFlashOpacity;
};

#endif // GAMEWIDGET_H