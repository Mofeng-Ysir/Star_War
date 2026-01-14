#include "CollisionSystem.h"

CollisionSystem::CollisionResult CollisionSystem::check(
    double heroX, double heroY, int heroW, int heroH,
    QList<Bullet> &bullets,
    QList<Enemy> &enemies,
    bool isLaserActive,
    bool isShieldActive,
    int currentPlaneId,
    int currentLevel,
    int totalWaves,
    int &progressCounter,
    const QImage &imgEnemy1,
    const QImage &imgEnemy3)
{
    CollisionResult result = {0, false, false, false, 0};

    QRect heroRect(heroX + 15, heroY + 15, heroW - 30, heroH - 30);
    QRect laserRect(heroX + heroW / 2 - 40, 0, 80, heroY);

    // --- 1. 激光判定 ---
    if (isLaserActive)
    {
        for (auto &b : bullets)
        {
            if (b.isEnemy && b.active && laserRect.contains(b.x, b.y))
            {
                b.active = false;
            }
        }
        for (auto &e : enemies)
        {
            if (!e.active)
                continue;
            QRect enemyRect;
            if (e.type == 10)
                enemyRect = QRect(e.x + 20, e.y + 20, 160, 110);
            else if (e.type == 2)
                enemyRect = QRect(e.x, e.y, imgEnemy3.width(), imgEnemy3.height());
            else
                enemyRect = QRect(e.x, e.y, imgEnemy1.width(), imgEnemy1.height());

            if (laserRect.intersects(enemyRect))
            {
                e.hp -= 2;
                if (e.hp <= 0)
                {
                    e.active = false;
                    if (e.type != 10 && progressCounter < totalWaves)
                        progressCounter++;
                    int add = (e.type == 10) ? (500 * currentLevel) : (e.type == 2 ? 50 : 10);
                    if (e.type == 10)
                        result.bossDied = true;
                    result.scoreAdded += add;
                }
            }
        }
    }

    // --- 2. 子弹判定 (数值同步) ---
    for (auto &b : bullets)
    {
        if (!b.active)
            continue;
        QRect bulletRect(b.x, b.y, 8, 8);

        if (b.isEnemy)
        {
            if (heroRect.intersects(bulletRect))
            {
                b.active = false;
                if (!isShieldActive)
                {
                    result.heroHit = true;
                    result.heroDamageTaken += 1;
                }
            }
        }
        else
        {
            for (auto &e : enemies)
            {
                if (!e.active)
                    continue;
                QRect enemyRect;
                if (e.type == 10)
                    enemyRect = QRect(e.x + 20, e.y + 20, 160, 110);
                else if (e.type == 2)
                    enemyRect = QRect(e.x, e.y, imgEnemy3.width(), imgEnemy3.height());
                else
                    enemyRect = QRect(e.x, e.y, imgEnemy1.width(), imgEnemy1.height());

                if (enemyRect.intersects(bulletRect))
                {
                    // 幻影(ID 3) 穿透
                    if (currentPlaneId != 3)
                        b.active = false;

                    // 【核心修改】根据 ID 设定伤害
                    int damage = 1;
                    switch (currentPlaneId)
                    {
                    case 0:
                        damage = 1;
                        break; // 勇者
                    case 1:
                        damage = 2;
                        break; // 双子 (2颗x2伤 = 4? 其实双子强在覆盖面)
                    case 2:
                        damage = 3;
                        break; // 泰坦 (3颗x3伤)
                    case 3:
                        damage = 4;
                        break; // 幻影 (面板写4)
                    case 4:
                        damage = 5;
                        break; // 虚空 (面板写5)
                    }
                    e.hp -= damage;

                    if (e.hp <= 0)
                    {
                        e.active = false;
                        if (e.type != 10 && progressCounter < totalWaves)
                            progressCounter++;
                        int add = (e.type == 10) ? (500 * currentLevel) : (e.type == 2 ? 50 : 10);
                        if (e.type == 10)
                            result.bossDied = true;
                        result.scoreAdded += add;
                    }
                    if (currentPlaneId != 3)
                        break;
                }
            }
        }
    }

    // --- 3. 身体撞击 ---
    for (auto &e : enemies)
    {
        if (!e.active)
            continue;
        QRect enemyRect;
        if (e.type == 10)
            enemyRect = QRect(e.x + 40, e.y + 40, 120, 80);
        else if (e.type == 2)
            enemyRect = QRect(e.x, e.y, imgEnemy3.width(), imgEnemy3.height());
        else
            enemyRect = QRect(e.x, e.y, imgEnemy1.width(), imgEnemy1.height());

        if (heroRect.intersects(enemyRect))
        {
            if (isShieldActive)
            {
                e.hp -= 1; // 护盾撞击只扣1
                if (e.hp <= 0)
                {
                    e.active = false;
                    if (e.type != 10 && progressCounter < totalWaves)
                        progressCounter++;
                    int add = (e.type == 10) ? (500 * currentLevel) : 20;
                    if (e.type == 10)
                        result.bossDied = true;
                    result.scoreAdded += add;
                }
            }
            else
            {
                if (e.type != 10)
                    e.active = false;
                result.heroHit = true;
                result.heroDamageTaken += 3;
                if (e.type == 10)
                    result.heroDamageTaken += 999;
            }
        }
    }

    return result;
}