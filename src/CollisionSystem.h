#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include "common.h"
#include <QList>
#include <QImage>
#include <QRect>

class CollisionSystem
{
public:
    // 返回值结构体，告诉 GameWidget 发生了什么
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
        bool isLaserActive,
        int currentLevel,
        int totalWaves,
        int &progressCounter,
        const QImage &imgEnemy1,
        const QImage &imgEnemy3);
};

#endif // COLLISIONSYSTEM_H