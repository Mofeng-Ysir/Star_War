#ifndef COMMON_H
#define COMMON_H

struct Bullet
{
    double x, y;
    double speedX, speedY;
    bool isEnemy;
    bool active;
};

struct Enemy
{
    int type; // 0:普通, 1:射击, 2:肉盾, 10:BOSS
    double x, y;
    int hp;
    int maxHp;
    bool active;
    int shootTimer;

    // Boss 专用
    double moveAngle;
    int bossId;
};

struct LevelConfig
{
    int levelId;
    int totalWaves;   // 召唤BOSS前的击杀进度需求
    int enemyHpScale; // 血量倍率
    int bossHp;       // Boss血量
};

#endif // COMMON_H