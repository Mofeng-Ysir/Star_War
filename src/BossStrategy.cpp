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
    // Level 6 旋转极快
    double rotSpeed = (boss.bossId == 6) ? (isEnraged ? 12.0 : 6.0) : (isEnraged ? 8.0 : 4.0);
    bossAttackAngle += rotSpeed;

    // 3. === 移动逻辑 ===
    // A. 进场
    if (boss.y < 50)
    {
        boss.y += 2.0;
        bossTargetPos = QPointF(width / 2 - 100, 100);
    }
    // B. 战斗移动
    else
    {
        // --- Level 6 特有移动：量子瞬移 ---
        if (boss.bossId == 6)
        {
            // 使用 bossTime 模拟计时 (bossTime 每一帧+0.03)
            // 每隔约 2-3 秒瞬移一次
            static double lastTeleportCheck = 0;

            if (bossTime > lastTeleportCheck + 4.0)
            { // 约2秒
                lastTeleportCheck = bossTime;

                // 随机选取屏幕上半部分的一个点
                double randX = QRandomGenerator::global()->bounded(50, width - 250);
                double randY = QRandomGenerator::global()->bounded(50, (int)(height * 0.5));
                bossTargetPos = QPointF(randX, randY);

                // 【瞬移效果】插值系数设为 0.3，实现快速闪现
                moveSpeedFactor = 0.3;
            }
            else
            {
                // 瞬移后的冷却期：几乎悬停不动
                moveSpeedFactor = 0.01;
            }
        }
        // --- 其他关卡：随机航点平滑飞行 ---
        else
        {
            double dist = -1;
            if (bossTargetPos.x() >= 0)
            {
                dist = qSqrt(qPow(boss.x - bossTargetPos.x(), 2) + qPow(boss.y - bossTargetPos.y(), 2));
            }
            // 到达目标或随机改变主意
            if (bossTargetPos.x() < 0 || dist < 20 || QRandomGenerator::global()->bounded(100) < 2)
            {
                double randX = QRandomGenerator::global()->bounded(20, width - 220);
                double randY = QRandomGenerator::global()->bounded(20, (int)(height * 0.6));
                bossTargetPos = QPointF(randX, randY);
            }
        }

        // 执行移动 (Lerp)
        boss.x = boss.x * (1.0 - moveSpeedFactor) + bossTargetPos.x() * moveSpeedFactor;
        boss.y = boss.y * (1.0 - moveSpeedFactor) + bossTargetPos.y() * moveSpeedFactor;
    }

    // 4. === 攻击逻辑 ===
    boss.shootTimer++;

    // --- Level 6: 终焉 (The End) ---
    if (boss.bossId == 6)
    {
        // 极快射速：狂暴时像泼水一样 (4帧一发)
        int rate = isEnraged ? 4 : 8;

        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;

            // 模式 A: 死亡莲花 (旋转全屏弹幕)
            // 每次发射 4 颗，顺时针旋转
            for (int k = 0; k < 4; k++)
            {
                Bullet b;
                b.x = boss.x + 100; // 中心
                b.y = boss.y + 75;
                b.isEnemy = true;
                b.active = true;

                // 角度 = 基础旋转角 + k*90度
                double deg = bossAttackAngle + k * 90.0;
                double rad = qDegreesToRadians(deg);

                b.speedX = qCos(rad) * 6.0;
                b.speedY = qSin(rad) * 6.0;
                bullets.append(b);
            }

            // 模式 B: 狂暴时追加 - 玩家定位狙击 (每隔几次触发)
            // 这里的条件必须用整数模运算
            if (isEnraged && (int(bossTime * 10) % 5 == 0))
            {
                Bullet b;
                b.x = boss.x + 100;
                b.y = boss.y + 75;
                b.isEnemy = true;
                b.active = true;

                double dx = (heroX + 30) - b.x;
                double dy = (heroY + 30) - b.y;
                double dist = qSqrt(dx * dx + dy * dy);
                // 超高速狙击弹 (速度12)
                b.speedX = (dx / dist) * 12.0;
                b.speedY = (dy / dist) * 12.0;
                bullets.append(b);
            }
        }
        return; // 第6关独立处理，直接返回
    }

    // --- 以下保持原有的 Level 1-5 逻辑 ---

    // Level 1: 狙击
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
    // Level 2: 加特林
    else if (boss.bossId == 2)
    {
        int rate = isEnraged ? 6 : 10;
        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;
            double sweepAngle = 30.0 * qSin(bossTime * 2.5);
            Bullet bL;
            bL.x = boss.x + 20;
            bL.y = boss.y + 120;
            bL.isEnemy = true;
            bL.active = true;
            double radL = qDegreesToRadians(90.0 + 15.0 + sweepAngle);
            bL.speedX = qCos(radL) * 7.0;
            bL.speedY = qSin(radL) * 7.0;
            bullets.append(bL);

            Bullet bR;
            bR.x = boss.x + 180;
            bR.y = boss.y + 120;
            bR.isEnemy = true;
            bR.active = true;
            double radR = qDegreesToRadians(90.0 - 15.0 - sweepAngle);
            bR.speedX = qCos(radR) * 7.0;
            bR.speedY = qSin(radR) * 7.0;
            bullets.append(bR);

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
    // Level 3: 螺旋
    else if (boss.bossId == 3)
    {
        int rate = isEnraged ? 3 : 5;
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
    // Level 4: 天降
    else if (boss.bossId == 4)
    {
        int rate = isEnraged ? 8 : 12;
        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;
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
    }
    // Level 5: 综合
    else
    {
        int rate = isEnraged ? 15 : 25;
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