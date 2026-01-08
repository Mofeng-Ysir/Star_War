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
    if (currentLevel >= max && currentLevel < 5)
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
    // 难度曲线
    config.totalWaves = 15 + (level * 5);
    config.enemyHpScale = level;
    config.bossHp = 50 * level + 50;
    return config;
}