#ifndef COMMON_H
#define COMMON_H

#include <QString>

// 飞机类型枚举
enum PlaneType
{
    PLANE_DEFAULT = 0, // 初始机 (激光)
    PLANE_DOUBLE = 1,  // 双子星 (双发 + 全屏弹幕)
    PLANE_SHOTGUN = 2, // 泰坦   (散弹 + 核弹清屏)
    PLANE_SNIPER = 3,  // 幻影   (穿透 + 无敌护盾)
    PLANE_ALIEN = 4    // 虚空   (追踪 + 时空冻结)
};

// 飞机属性结构体
struct PlaneStats
{
    int id;
    QString name;
    QString desc;
    int cost;
    int hp;
    double speed;
};

// 子弹结构体
struct Bullet
{
    double x, y;
    double speedX, speedY;
    bool isEnemy;
    bool active;
    int hitCount = 0; // 【关键】穿透计数，没有这个变量会导致 CollisionSystem 报错
};

// 敌人结构体
struct Enemy
{
    int type; // 0:普通, 1:射击, 2:肉盾, 10:BOSS
    double x, y;
    int hp;
    int maxHp;
    bool active;
    int shootTimer;
    double moveAngle;
    int bossId;
};

// 关卡配置结构体
struct LevelConfig
{
    int levelId;
    int totalWaves;   // 召唤BOSS前的击杀进度需求
    int enemyHpScale; // 小怪血量倍率
    int bossHp;       // Boss血量
};

#endif // COMMON_H