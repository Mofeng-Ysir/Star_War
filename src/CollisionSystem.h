#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include "common.h"
#include <QList>
#include <QImage>
#include <QRect>

class CollisionSystem
{
public:
    struct CollisionResult
    {
        int scoreAdded;
        bool bossHit;
        bool heroHit;
        bool bossDied;
        int heroDamageTaken;
    };

    static CollisionResult check(
        double heroX, double heroY, int heroW, int heroH,
        QList<Bullet> &bullets,
        QList<Enemy> &enemies,
        bool isLaserActive,  // 是否激光
        bool isShieldActive, // 【新增】是否开盾
        int currentPlaneId,  // 【新增】当前飞机ID (用于判断追踪弹伤害)
        int currentLevel,
        int totalWaves,
        int &progressCounter,
        const QImage &imgEnemy1,
        const QImage &imgEnemy3);
};

#endif // COLLISIONSYSTEM_H