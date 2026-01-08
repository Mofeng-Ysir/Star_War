#ifndef COMMON_H
#define COMMON_H

#include <QString>

// 飞机类型枚举
enum PlaneType
{
    PLANE_DEFAULT = 0, // 初始机 (激光)
    PLANE_DOUBLE = 1,  // 双子星 (双发 + 全屏弹幕)
    PLANE_SHOTGUN = 2, // 泰坦   (散弹 + 核弹清屏)
    PLANE_SNIPER = 3,  // 幻影   (极速 + 无敌护盾)
    PLANE_ALIEN = 4    // 虚空   (追踪 + 时空冻结)
};

// 飞机属性结构体
struct PlaneStats
{
    int id;
    QString name;
    QString desc; // 描述
    int cost;     // 价格
    int hp;       // 基础血量
    double speed; // 移动速度
};

// ... (Bullet, Enemy, LevelConfig 保持不变)
struct Bullet
{
    double x, y;
    double speedX, speedY;
    bool isEnemy;
    bool active;
};

struct Enemy
{
    int type;
    double x, y;
    int hp;
    int maxHp;
    bool active;
    int shootTimer;
    double moveAngle;
    int bossId;
};

struct LevelConfig
{
    int levelId;
    int totalWaves;
    int enemyHpScale;
    int bossHp;
};

#endif // COMMON_H