#ifndef LEVELMANAGER_H
#define LEVELMANAGER_H

#include "common.h"
#include <QString>

class LevelManager
{
public:
    static int getMaxUnlockedLevel();
    static void unlockNextLevel(int currentLevel);
    static LevelConfig getLevelConfig(int level);

private:
    static QString getSavePath();
};

#endif // LEVELMANAGER_H