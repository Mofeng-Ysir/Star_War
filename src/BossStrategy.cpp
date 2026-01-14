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
    // === 1. 全局状态更新 ===
    // 狂暴判定：血量低于 60% 即进入狂暴，而不是50%，增加压迫感时长
    bool isEnraged = (boss.hp < (boss.maxHp * 0.6));

    // 时间流逝：狂暴后时间流逝变快，导致正弦波移动和旋转更鬼畜
    bossTime += (isEnraged ? 0.06 : 0.03);

    // 基础旋转角：用于螺旋弹幕
    bossAttackAngle += (isEnraged ? 15.0 : 5.0);

    // 技能预警时，BOSS 几乎静止，给玩家压迫感
    double currentMoveSpeed = isEnraged ? 0.08 : 0.04; // 移动速度翻倍
    if (boss.isWarning)
        currentMoveSpeed = 0.005;

    // === 2. 移动逻辑 (更具侵略性) ===
    // A. 进场
    if (boss.y < 80)
    {
        boss.y += 3.0; // 快速进场
        bossTargetPos = QPointF(width / 2 - 100, 100);
    }
    // B. 战斗移动
    else
    {
        // Level 6 & Level 4 (高机动型BOSS) 使用瞬移或大幅度机动
        if (boss.bossId == 6 || boss.bossId == 4)
        {
            static double lastTeleportCheck = 0;
            // 狂暴后瞬移频率极高 (1.5秒一次)
            double teleportCD = isEnraged ? 1.5 : 3.0;

            if (bossTime > lastTeleportCheck + teleportCD * 10)
            { // *10是因为bossTime增长快
                lastTeleportCheck = bossTime;

                // 随机移动到玩家头顶附近，而不是随机乱飞
                double targetX = heroX + QRandomGenerator::global()->bounded(200) - 100;
                double targetY = QRandomGenerator::global()->bounded(50, (int)(height * 0.4));

                // 边界限制
                if (targetX < 50)
                    targetX = 50;
                if (targetX > width - 250)
                    targetX = width - 250;

                bossTargetPos = QPointF(targetX, targetY);
                currentMoveSpeed = 0.2; // 极速位移
            }
        }
        // 其他 BOSS 使用平滑随机巡航
        else
        {
            double dist = -1;
            if (bossTargetPos.x() >= 0)
            {
                dist = qSqrt(qPow(boss.x - bossTargetPos.x(), 2) + qPow(boss.y - bossTargetPos.y(), 2));
            }
            // 更加频繁地更换位置，让玩家难以瞄准
            if (bossTargetPos.x() < 0 || dist < 30 || QRandomGenerator::global()->bounded(100) < 3)
            {
                double randX = QRandomGenerator::global()->bounded(20, width - 220);
                double randY = QRandomGenerator::global()->bounded(20, (int)(height * 0.35));
                bossTargetPos = QPointF(randX, randY);
            }
        }
        // 执行移动
        boss.x = boss.x * (1.0 - currentMoveSpeed) + bossTargetPos.x() * currentMoveSpeed;
        boss.y = boss.y * (1.0 - currentMoveSpeed) + bossTargetPos.y() * currentMoveSpeed;
    }

    // === 3. 攻击逻辑 (地狱绘图开始) ===
    boss.shootTimer++;

    // ---------------------------------------------------------
    // Level 1: 狙击手 -> 改为 "光束刺客"
    // 技能：不仅冲撞，冲撞路径上还会残留子弹
    // ---------------------------------------------------------
    if (boss.bossId == 1)
    {
        // 1. 技能：致命冲撞
        boss.skillTimer++;
        int skillCD = isEnraged ? 180 : 300; // 3~5秒一次冲撞

        if (boss.state == STATE_NORMAL && boss.skillTimer > skillCD)
        {
            boss.skillTimer = 0;
            boss.state = STATE_WARNING;
            boss.isWarning = true;
            boss.attackTargetX = heroX + 25; // 预判一点点
            boss.attackTargetY = heroY + 25;

            // 计算冲刺向量
            double dx = boss.attackTargetX - (boss.x + 100);
            double dy = boss.attackTargetY - (boss.y + 100);
            double dist = qSqrt(dx * dx + dy * dy);
            boss.dashSpeedX = (dx / dist) * 25.0; // 极速冲刺
            boss.dashSpeedY = (dy / dist) * 25.0;
        }
        else if (boss.state == STATE_WARNING)
        {
            // 预警时间缩短到 0.8秒
            if (boss.skillTimer > 45)
            {
                boss.skillTimer = 0;
                boss.isWarning = false;
                boss.state = STATE_SKILL_DASH;
            }
        }
        else if (boss.state == STATE_SKILL_DASH)
        {
            boss.x += boss.dashSpeedX;
            boss.y += boss.dashSpeedY;

            // 【新机制】冲刺路径撒雷
            if (boss.skillTimer % 2 == 0)
            {
                Bullet b;
                b.x = boss.x + 100;
                b.y = boss.y + 100;
                b.active = true;
                b.isEnemy = true;
                b.speedX = 0;
                b.speedY = 0;       // 这是一个地雷，不动
                b.isSpecial = true; // 特殊外观
                bullets.append(b);
            }

            boss.skillTimer++;
            if (boss.skillTimer > 15 || boss.y > height)
                boss.state = STATE_RECOVERY;
        }
        else if (boss.state == STATE_RECOVERY)
        {
            boss.skillTimer++;
            // 慢慢回位
            if (boss.y > 150)
                boss.y -= 4.0;
            if (boss.skillTimer > 60)
                boss.state = STATE_NORMAL;
        }
        // 2. 普攻：扇形封锁
        else if (boss.state == STATE_NORMAL)
        {
            int rate = isEnraged ? 40 : 60;
            if (boss.shootTimer > rate)
            {
                boss.shootTimer = 0;
                int count = isEnraged ? 7 : 5; // 狂暴7发扇形
                double startAngle = 60.0;
                double step = 60.0 / (count - 1);
                for (int i = 0; i < count; i++)
                {
                    Bullet b;
                    b.x = boss.x + 100;
                    b.y = boss.y + 150;
                    b.active = true;
                    b.isEnemy = true;
                    double rad = qDegreesToRadians(startAngle + i * step);
                    b.speedX = qCos(rad) * 9.0;
                    b.speedY = qSin(rad) * 9.0;
                    bullets.append(b);
                }
            }
        }
    }

    // ---------------------------------------------------------
    // Level 2: 重装机甲 -> 改为 "弹幕要塞"
    // 攻击：不再有空隙，而是像雨刮器一样无死角扫射
    // ---------------------------------------------------------
    else if (boss.bossId == 2)
    {
        int rate = isEnraged ? 4 : 8; // 极快射速
        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;

            // 扫射逻辑：左右摇摆
            double sweep = 45.0 * qSin(bossTime * 3.0);

            // 左炮：慢速大球
            Bullet b1;
            b1.x = boss.x;
            b1.y = boss.y + 100;
            b1.active = true;
            b1.isEnemy = true;
            b1.isSpecial = true;
            double r1 = qDegreesToRadians(90.0 + sweep);
            b1.speedX = qCos(r1) * 5.0;
            b1.speedY = qSin(r1) * 5.0;
            bullets.append(b1);

            // 右炮：高速小弹
            Bullet b2;
            b2.x = boss.x + 200;
            b2.y = boss.y + 100;
            b2.active = true;
            b2.isEnemy = true;
            double r2 = qDegreesToRadians(90.0 - sweep);
            b2.speedX = qCos(r2) * 9.0;
            b2.speedY = qSin(r2) * 9.0;
            bullets.append(b2);

            // 狂暴：全屏炸裂
            if (isEnraged && (int)(bossTime * 10) % 15 == 0)
            {
                for (int k = 0; k < 12; k++)
                {
                    Bullet b;
                    b.x = boss.x + 100;
                    b.y = boss.y + 100;
                    b.active = true;
                    b.isEnemy = true;
                    double rad = qDegreesToRadians(k * 30.0 + bossAttackAngle);
                    b.speedX = qCos(rad) * 6.0;
                    b.speedY = qSin(rad) * 6.0;
                    bullets.append(b);
                }
            }
        }
    }

    // ---------------------------------------------------------
    // Level 3: 曼陀罗 -> 改为 "死亡螺旋"
    // 攻击：正反双螺旋，没有死角的弹幕海
    // ---------------------------------------------------------
    else if (boss.bossId == 3)
    {
        int rate = 4; // 机关枪射速
        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;

            // 顺时针螺旋
            int arms = isEnraged ? 4 : 3;
            for (int k = 0; k < arms; k++)
            {
                Bullet b;
                b.x = boss.x + 100;
                b.y = boss.y + 100;
                b.active = true;
                b.isEnemy = true;
                double rad = qDegreesToRadians(bossAttackAngle + k * (360.0 / arms));
                b.speedX = qCos(rad) * 6.0;
                b.speedY = qSin(rad) * 6.0;
                bullets.append(b);
            }

            // 狂暴：叠加逆时针螺旋
            if (isEnraged)
            {
                for (int k = 0; k < arms; k++)
                {
                    Bullet b;
                    b.x = boss.x + 100;
                    b.y = boss.y + 100;
                    b.active = true;
                    b.isEnemy = true;
                    b.isSpecial = true; // 特殊颜色区分
                    double rad = qDegreesToRadians(-bossAttackAngle * 1.5 + k * (360.0 / arms));
                    b.speedX = qCos(rad) * 7.0;
                    b.speedY = qSin(rad) * 7.0;
                    bullets.append(b);
                }
            }
        }
    }

    // ---------------------------------------------------------
    // Level 4: 轰炸机 -> 改为 "地毯式清洗"
    // 攻击：预警时间极短，安全区极小
    // ---------------------------------------------------------
    else if (boss.bossId == 4)
    {
        // 状态机控制：预警 -> 轰炸 -> 休息
        if (boss.state == STATE_NORMAL)
        {
            boss.skillTimer++;
            if (boss.skillTimer > 60)
            { // 仅仅1秒就准备下一轮
                boss.skillTimer = 0;
                boss.state = STATE_WARNING;
                boss.isWarning = true;
                // 随机生成 3-4 个轰炸条，只有一个空隙
                boss.attackTargetX = QRandomGenerator::global()->bounded(width); // 记录空隙位置X

                // 构造一个全屏宽度的警告矩形，视觉上由GameWidget去画多条红带
                // 为了简单，我们只标记要轰炸的状态，具体子弹生成在下一阶段
                boss.warningRect = QRect(0, 0, width, height);
            }
        }
        else if (boss.state == STATE_WARNING)
        {
            boss.skillTimer++;
            if (boss.skillTimer > 40)
            { // 0.6秒预警，考验反应
                boss.skillTimer = 0;
                boss.isWarning = false;
                boss.warningRect = QRect();
                boss.state = STATE_SKILL_FIRE;
            }
        }
        else if (boss.state == STATE_SKILL_FIRE)
        {
            // 一次性生成密集弹幕墙
            int gapX = (int)boss.attackTargetX;
            int gapWidth = 120; // 安全区宽度

            for (int i = 0; i < width; i += 25)
            { // 每25像素一颗子弹
                if (i > gapX - gapWidth / 2 && i < gapX + gapWidth / 2)
                    continue; // 留出空隙

                Bullet b;
                b.x = i;
                b.y = -20;
                b.active = true;
                b.isEnemy = true;
                b.speedX = 0;
                b.speedY = isEnraged ? 12.0 : 8.0; // 高速下落
                bullets.append(b);
            }
            boss.state = STATE_NORMAL; // 立即开始下一轮循环
        }

        // 始终发射的干扰弹
        if (boss.shootTimer++ > 30)
        {
            boss.shootTimer = 0;
            Bullet b;
            b.x = boss.x + 100;
            b.y = boss.y + 150;
            b.active = true;
            b.isEnemy = true;
            b.isSpecial = true;
            double dx = heroX - b.x;
            double dy = heroY - b.y;
            double dist = qSqrt(dx * dx + dy * dy);
            b.speedX = (dx / dist) * 6.0;
            b.speedY = (dy / dist) * 6.0;
            bullets.append(b);
        }
    }

    // ---------------------------------------------------------
    // Level 5 & 6: 终极形态 (保留并加强)
    // ---------------------------------------------------------
    else
    {
        int rate = isEnraged ? 4 : 8;
        if (boss.shootTimer > rate)
        {
            boss.shootTimer = 0;

            // 1. 核心散射 (旋转)
            for (int k = 0; k < 5; k++)
            {
                Bullet b;
                b.x = boss.x + 100;
                b.y = boss.y + 100;
                b.active = true;
                b.isEnemy = true;
                double rad = qDegreesToRadians(bossAttackAngle + k * 72.0);
                b.speedX = qCos(rad) * 7.0;
                b.speedY = qSin(rad) * 7.0;
                bullets.append(b);
            }

            // 2. 追踪弹 (每隔几次)
            if ((int)(bossTime * 10) % 5 == 0)
            {
                Bullet b;
                b.x = boss.x + 100;
                b.y = boss.y + 100;
                b.active = true;
                b.isEnemy = true;
                b.isSpecial = true;
                double dx = heroX - b.x;
                double dy = heroY - b.y;
                double dist = qSqrt(dx * dx + dy * dy);
                b.speedX = (dx / dist) * 11.0;
                b.speedY = (dy / dist) * 11.0;
                bullets.append(b);
            }

            // 3. Level 6 专属：全屏随机弹 (让场面更乱)
            if (boss.bossId == 6 && isEnraged)
            {
                Bullet b;
                b.x = QRandomGenerator::global()->bounded(width);
                b.y = -10;
                b.active = true;
                b.isEnemy = true;
                b.speedX = (QRandomGenerator::global()->bounded(10) - 5) / 2.0;
                b.speedY = 6.0;
                bullets.append(b);
            }
        }
    }
}