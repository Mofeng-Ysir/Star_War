#include "CollisionSystem.h"

CollisionSystem::CollisionResult CollisionSystem::check(
    double heroX, double heroY, int heroW, int heroH,
    QList<Bullet> &bullets,
    QList<Enemy> &enemies,
    bool isLaserActive,
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
        // 消弹
        for (auto &b : bullets)
        {
            if (b.isEnemy && b.active && laserRect.contains(b.x, b.y))
            {
                b.active = false;
            }
        }
        // 伤敌
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

                    int add = 10;
                    if (e.type == 10)
                    {
                        add = 500 * currentLevel;
                        result.bossDied = true;
                    }
                    else if (e.type == 2)
                        add = 50;
                    result.scoreAdded += add;
                }
            }
        }
    }

    // --- 2. 子弹判定 ---
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
                result.heroHit = true;
                result.heroDamageTaken += 1;
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
                    b.active = false;
                    e.hp--;
                    if (e.hp <= 0)
                    {
                        e.active = false;
                        if (e.type != 10 && progressCounter < totalWaves)
                            progressCounter++;
                        int add = 10;
                        if (e.type == 10)
                        {
                            add = 500 * currentLevel;
                            result.bossDied = true;
                        }
                        else if (e.type == 2)
                            add = 50;
                        result.scoreAdded += add;
                    }
                    break;
                }
            }
        }
    }

    // --- 3. 撞击判定 ---
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
            e.active = false;
            result.heroHit = true;
            result.heroDamageTaken += 3;
            if (e.type == 10)
            {
                result.heroDamageTaken += 100; // 撞Boss直接死
                result.bossDied = true;        // Boss也算死
            }
        }
    }

    return result;
}