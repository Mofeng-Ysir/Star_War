#ifndef BOSSSTRATEGY_H
#define BOSSSTRATEGY_H

#include "common.h"
#include <QList>
#include <QPointF>

class BossStrategy
{
public:
    BossStrategy();

    // 重置状态（每次新关卡开始时调用）
    void reset();

    // 核心更新函数
    void update(Enemy &boss,
                QList<Bullet> &bullets,
                double heroX, double heroY,
                int screenWidth, int screenHeight,
                const LevelConfig &config);

private:
    double bossTime;
    double bossAttackAngle;
    QPointF bossTargetPos; // 用于随机航点移动
};

#endif // BOSSSTRATEGY_H