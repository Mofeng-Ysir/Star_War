#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "common.h"
#include <QList>

class DataManager
{
public:
    static void loadData();
    static void saveData();

    static int getCoins();
    static void addCoins(int amount);
    static bool spendCoins(int amount);

    static int getCurrentPlaneId();
    static void setCurrentPlane(int id);

    static bool isPlaneUnlocked(int id);
    static void unlockPlane(int id);

    static PlaneStats getPlaneStats(int id);

private:
    static int m_coins;
    static int m_currentPlaneId;
    static QList<bool> m_unlockedPlanes; // true代表已解锁
    static QString getFilePath();
};

#endif // DATAMANAGER_H