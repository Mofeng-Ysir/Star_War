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

    // --- 1. 激光 ---
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

    // --- 2. 子弹 ---
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
                    // 【核心修改】幻影战机(ID=3) 子弹穿透
                    if (currentPlaneId != 3)
                        b.active = false;

                    e.hp -= 1; // 统一伤害，靠射速或穿透打输出

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
                // 【核心修改】护盾撞击伤害改为 1 (每帧)
                e.hp -= 1;
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