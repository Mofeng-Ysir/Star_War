#include "LevelManager.h"
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

QString LevelManager::getSavePath()
{
    return QCoreApplication::applicationDirPath() + "/level_save.txt";
}

int LevelManager::getMaxUnlockedLevel()
{
    QFile file(getSavePath());
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        int lvl;
        in >> lvl;
        return lvl > 0 ? lvl : 1;
    }
    return 1;
}

void LevelManager::unlockNextLevel(int currentLevel)
{
    int max = getMaxUnlockedLevel();
    // 【核心修改】允许解锁到 < 6 (也就是通关5解锁6)
    // 如果通关了6，就不再解锁了（或者你可以写 < 7 开启二周目）
    if (currentLevel >= max && currentLevel < 6)
    {
        QFile file(getSavePath());
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out << (currentLevel + 1);
        }
    }
}

LevelConfig LevelManager::getLevelConfig(int level)
{
    LevelConfig config;
    config.levelId = level;

    if (level == 6)
    {
        // === 第 6 关：最终挑战 ===
        config.totalWaves = 100; // 漫长的战役
        config.enemyHpScale = 8; // 小怪血量极高
        config.bossHp = 2500;    // 最终BOSS血量 (对比第5关 500)
    }
    else
    {
        // 普通关卡
        config.totalWaves = 15 + (level * 5);
        config.enemyHpScale = level;
        config.bossHp = 80 * level + 100;
    }
    return config;
}