#include "BossStrategy.h"
#include <QtMath>
#include <QRandomGenerator>

BossStrategy::BossStrategy()
{
    reset();
}

void BossStrategy::reset()
{
    bossTime = 0;
    bossAttackAngle = 0;
    bossTargetPos = QPointF(-1, -1);
}

void BossStrategy::update(Enemy &boss, QList<Bullet> &bullets,
                          double heroX, double heroY,
                          int width, int height,
                          const LevelConfig &config)
{
    // 1. 状态判断
    bool isEnraged = (boss.hp < (boss.maxHp / 2));
    double moveSpeedFactor = isEnraged ? 0.05 : 0.02;

    // 2. 时间更新
    bossTime += 0.03;
    bossAttackAngle += (isEnraged ? 8.0 : 4.0);

    // 3. === 移动逻辑 (随机航点) ===
    // A. 进场
    if (boss.y < 50)
    {
        boss.y += 2.0;
        bossTargetPos = QPointF(width / 2 - 100, 100);
    }
    // B. 战斗移动
    else
    {
        double dist = -1;
        if (bossTargetPos.x() >= 0)
        {
            dist = qSqrt(qPow(boss.x - bossTargetPos.x(), 2) + qPow(boss.y - bossTargetPos.y(), 2));
        }

        // 选取新目标点的条件：无目标、到达目标、或极小概率随机变向
        if (bossTargetPos.x() < 0 || dist < 20 || QRandomGenerator::global()->bounded(100) < 2)
        {
            double randX = QRandomGenerator::global()->bounded(20, width - 220);
            double randY = QRandomGenerator::global()->bounded(20, (int)(height * 0.6));
            bossTargetPos = QPointF(randX, randY);
        }

        // 平滑插值移动
        boss.x = boss.x * (1.0 - moveSpeedFactor) + bossTargetPos.x() * moveSpeedFactor;
        boss.y = boss.y * (1.0 - moveSpeedFactor) + bossTargetPos.y() * moveSpeedFactor;
    }

    // 4. === 攻击逻辑 ===
    boss.shootTimer++;
    int baseRate = 50 - (config.levelId * 5);
    if (isEnraged)
        baseRate /= 2;
    if (baseRate < 10)
        baseRate = 10;

    if (boss.shootTimer > baseRate)
    {
        boss.shootTimer = 0;

        // Level 1: 狙击
        if (boss.bossId == 1)
        {
            Bullet b;
            b.x = boss.x + 100;
            b.y = boss.y + 150;
            b.isEnemy = true;
            b.active = true;
            double dx = (heroX + 30) - b.x;
            double dy = (heroY + 30) - b.y;
            double dist = qSqrt(dx * dx + dy * dy);
            b.speedX = (dx / dist) * 8.0;
            b.speedY = (dy / dist) * 8.0;
            bullets.append(b);
            if (isEnraged)
            {
                for (int k : {-1, 1})
                {
                    Bullet b2 = b;
                    b2.x += k * 40;
                    bullets.append(b2);
                }
            }
        }
        // Level 2: 霰弹
        else if (boss.bossId == 2)
        {
            int numBullets = isEnraged ? 7 : 5;
            for (int k = -(numBullets / 2); k <= (numBullets / 2); k++)
            {
                Bullet b;
                b.x = boss.x + 100;
                b.y = boss.y + 150;
                b.isEnemy = true;
                b.active = true;
                b.speedX = k * 2.0;
                b.speedY = 7.0;
                bullets.append(b);
            }
        }
        // Level 3: 螺旋
        else if (boss.bossId == 3)
        {
            int arms = isEnraged ? 4 : 2;
            for (int k = 0; k < arms; k++)
            {
                Bullet b;
                b.x = boss.x + 100;
                b.y = boss.y + 100;
                b.isEnemy = true;
                b.active = true;
                double rad = qDegreesToRadians(bossAttackAngle + k * (360 / arms));
                b.speedX = qCos(rad) * 6.0;
                b.speedY = qSin(rad) * 6.0;
                bullets.append(b);
            }
        }
        // Level 4: 天降正义
        else if (boss.bossId == 4)
        {
            int count = isEnraged ? 3 : 1;
            for (int k = 0; k < count; k++)
            {
                Bullet b;
                b.x = QRandomGenerator::global()->bounded(width);
                b.y = -20;
                b.isEnemy = true;
                b.active = true;
                b.speedX = 0;
                b.speedY = 8.0 + QRandomGenerator::global()->bounded(5);
                bullets.append(b);
            }
            Bullet b;
            b.x = boss.x + 100;
            b.y = boss.y + 150;
            b.isEnemy = true;
            b.active = true;
            b.speedX = 0;
            b.speedY = 7.0;
            bullets.append(b);
        }
        // Level 5: 综合
        else
        {
            Bullet b;
            b.x = boss.x + 100;
            b.y = boss.y + 150;
            b.isEnemy = true;
            b.active = true;
            double dx = (heroX + 30) - b.x;
            double dy = (heroY + 30) - b.y;
            double dist = qSqrt(dx * dx + dy * dy);
            b.speedX = (dx / dist) * 9.0;
            b.speedY = (dy / dist) * 9.0;
            bullets.append(b);
            if (isEnraged && (int(bossTime * 10) % 4 == 0))
            {
                for (int k = 0; k < 8; k++)
                {
                    Bullet fan;
                    fan.x = boss.x + 100;
                    fan.y = boss.y + 100;
                    fan.isEnemy = true;
                    fan.active = true;
                    double rad = qDegreesToRadians(k * 45.0);
                    fan.speedX = qCos(rad) * 4.0;
                    fan.speedY = qSin(rad) * 4.0;
                    bullets.append(fan);
                }
            }
        }
    }
}