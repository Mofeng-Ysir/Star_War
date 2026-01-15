#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "common.h"
#include <QList>
#include <QMap>

class DataManager
{
public:
    static void loadData();
    static void saveData();

    // 基础资源
    static int getCoins();
    static void addCoins(int amount);
    static bool spendCoins(int amount);

    // 战机相关
    static int getCurrentPlaneId();
    static void setCurrentPlane(int id);
    static bool isPlaneUnlocked(int id);
    static void unlockPlane(int id);
    static PlaneStats getPlaneStats(int id);

    // --- 【新增】装备相关 ---
    static QList<int> getInventory(); // 获取背包中所有装备ID
    static void addEquipment(int equipId);
    static bool hasEquipment(int equipId);

    // 穿戴系统
    static int getEquippedId(EquipType type); // 获取某个部位当前装备的ID (-1为空)
    static void equipItem(EquipType type, int equipId);

    // 装备数据库 & 生成
    static Equipment getEquipmentById(int id);
    static int generateDrop(int bossId); // 根据BOSS掉落装备ID

    // 获取最终玩家属性 (战机 + 装备)
    static PlaneStats getFinalStats(int planeId);

private:
    static int m_coins;
    static int m_currentPlaneId;
    static QList<bool> m_unlockedPlanes;

    // 新增存储字段
    static QList<int> m_inventory; // 拥有的装备ID列表
    static int m_equippedCore;     // 当前核心ID
    static int m_equippedArmor;    // 当前装甲ID
    static int m_equippedEngine;   // 当前引擎ID

    static QString getFilePath();
};

#endif // DATAMANAGER_H