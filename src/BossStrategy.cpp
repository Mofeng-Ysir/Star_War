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

        // 选取新目标点的条件
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

    // 4. === 攻击逻辑 (重构版：每个Boss独立管理射速) ===
    boss.shootTimer++;

    // --- Level 1: 狙击手 ---
    if (boss.bossId == 1)
    {
        int rate = isEnraged ? 40 : 60;
        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;
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
    }
    // --- Level 2: 重装机甲 (交叉火力) ---
    else if (boss.bossId == 2)
    {
        // 极快射速：普通10帧，狂暴6帧
        int rate = isEnraged ? 6 : 10;

        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;

            // 扫射摆动角度
            double sweepAngle = 30.0 * qSin(bossTime * 2.5);

            // 左翼炮口
            Bullet bLeft;
            bLeft.x = boss.x + 20;
            bLeft.y = boss.y + 120;
            bLeft.isEnemy = true;
            bLeft.active = true;
            double radL = qDegreesToRadians(90.0 + 15.0 + sweepAngle);
            bLeft.speedX = qCos(radL) * 7.0;
            bLeft.speedY = qSin(radL) * 7.0;
            bullets.append(bLeft);

            // 右翼炮口
            Bullet bRight;
            bRight.x = boss.x + 180;
            bRight.y = boss.y + 120;
            bRight.isEnemy = true;
            bRight.active = true;
            double radR = qDegreesToRadians(90.0 - 15.0 - sweepAngle);
            bRight.speedX = qCos(radR) * 7.0;
            bRight.speedY = qSin(radR) * 7.0;
            bullets.append(bRight);

            // 狂暴：附加环形震荡 (独立计时，不依赖 shootTimer)
            // 利用 bossTime 取整来做低频触发
            if (isEnraged && (int(bossTime * 10) % 8 == 0))
            {
                for (int k = 0; k < 12; k++)
                {
                    Bullet b;
                    b.x = boss.x + 100;
                    b.y = boss.y + 100;
                    b.isEnemy = true;
                    b.active = true;
                    double rad = qDegreesToRadians(k * 30.0);
                    b.speedX = qCos(rad) * 4.0;
                    b.speedY = qSin(rad) * 4.0;
                    bullets.append(b);
                }
            }
        }
    }
    // --- Level 3: 螺旋弹幕 ---
    else if (boss.bossId == 3)
    {
        int rate = isEnraged ? 3 : 5; // 极快
        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;
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
    }
    // --- Level 4: 天降正义 ---
    else if (boss.bossId == 4)
    {
        int rate = isEnraged ? 8 : 12;
        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;
            int count = isEnraged ? 3 : 1;
            // 随机落雷
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
            // 正面直射
            Bullet b;
            b.x = boss.x + 100;
            b.y = boss.y + 150;
            b.isEnemy = true;
            b.active = true;
            b.speedX = 0;
            b.speedY = 7.0;
            bullets.append(b);
        }
    }
    // --- Level 5: 综合地狱 ---
    else
    {
        int rate = isEnraged ? 15 : 25;
        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;

            // 追踪弹
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

            // 狂暴环形弹
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