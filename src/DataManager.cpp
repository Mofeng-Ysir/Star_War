#include "DataManager.h"
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QRandomGenerator>

// 初始化静态成员
int DataManager::m_coins = 0;
int DataManager::m_currentPlaneId = 0;
QList<bool> DataManager::m_unlockedPlanes = {true, false, false, false, false};
QList<int> DataManager::m_inventory = {};
int DataManager::m_equippedCore = -1;
int DataManager::m_equippedArmor = -1;
int DataManager::m_equippedEngine = -1;

QString DataManager::getFilePath()
{
    return QCoreApplication::applicationDirPath() + "/save_v2.dat"; // 升级存档文件版本
}

// 存档格式：金币|飞机ID|飞机掩码|核心ID|装甲ID|引擎ID|背包ID1,背包ID2...
void DataManager::loadData()
{
    QFile file(getFilePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        QString line = in.readLine();
        QStringList parts = line.split("|");
        if (parts.size() >= 7)
        {
            m_coins = parts[0].toInt();
            m_currentPlaneId = parts[1].toInt();
            QString mask = parts[2];
            for (int i = 0; i < 5 && i < mask.length(); ++i)
                m_unlockedPlanes[i] = (mask[i] == '1');

            m_equippedCore = parts[3].toInt();
            m_equippedArmor = parts[4].toInt();
            m_equippedEngine = parts[5].toInt();

            m_inventory.clear();
            QStringList inv = parts[6].split(",");
            for (const QString &s : inv)
                if (!s.isEmpty())
                    m_inventory.append(s.toInt());
        }
        file.close();
    }
}

void DataManager::saveData()
{
    QFile file(getFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        QString mask = "";
        for (bool b : m_unlockedPlanes)
            mask += (b ? "1" : "0");

        QString invStr = "";
        for (int id : m_inventory)
            invStr += QString::number(id) + ",";

        out << m_coins << "|" << m_currentPlaneId << "|" << mask << "|"
            << m_equippedCore << "|" << m_equippedArmor << "|" << m_equippedEngine << "|"
            << invStr;
        file.close();
    }
}

// ... (getCoins, addCoins, spendCoins, Plane相关函数保持不变，请复制之前的代码) ...
int DataManager::getCoins() { return m_coins; }
void DataManager::addCoins(int amount)
{
    m_coins += amount;
    saveData();
}
bool DataManager::spendCoins(int amount)
{
    if (m_coins >= amount)
    {
        m_coins -= amount;
        saveData();
        return true;
    }
    return false;
}
int DataManager::getCurrentPlaneId() { return m_currentPlaneId; }
void DataManager::setCurrentPlane(int id)
{
    m_currentPlaneId = id;
    saveData();
}
bool DataManager::isPlaneUnlocked(int id)
{
    if (id < 0 || id >= m_unlockedPlanes.size())
        return false;
    return m_unlockedPlanes[id];
}
void DataManager::unlockPlane(int id)
{
    if (id >= 0 && id < m_unlockedPlanes.size())
    {
        m_unlockedPlanes[id] = true;
        saveData();
    }
}

PlaneStats DataManager::getPlaneStats(int id)
{
    PlaneStats s;
    s.id = id;

    switch (id)
    {
    case 0: // 勇者号 - 均衡型
        s.name = "勇者号";
        s.desc = "标准战斗机，各项性能均衡。新手推荐。";
        s.hp = 4;
        s.speed = 1.0;
        s.viewAtk = 2;
        s.viewDef = 2;
        s.viewRate = 1.0;
        s.viewHp = 4;
        s.cost = 0; // 初始拥有
        break;
    case 1: // 双子星 - 双火力型
        s.name = "双子星";
        s.desc = "配备双发炮台，火力强劲。进攻型战机。";
        s.hp = 3;
        s.speed = 1.2;
        s.viewAtk = 4;
        s.viewDef = 1;
        s.viewRate = 1.5;
        s.viewHp = 3;
        s.cost = 500;
        break;
    case 2: // 泰坦 - 重装甲型
        s.name = "泰坦";
        s.desc = "装甲厚重，防御力优秀。坦克型战机。";
        s.hp = 8;
        s.speed = 0.7;
        s.viewAtk = 2;
        s.viewDef = 5;
        s.viewRate = 0.8;
        s.viewHp = 8;
        s.cost = 800;
        break;
    case 3: // 幻影 - 高速型
        s.name = "幻影";
        s.desc = "超高速机动，闪避能力强。灵活型战机。";
        s.hp = 2;
        s.speed = 1.8;
        s.viewAtk = 3;
        s.viewDef = 1;
        s.viewRate = 1.8;
        s.viewHp = 2;
        s.cost = 1200;
        break;
    case 4: // 虚空 - 超强型
        s.name = "虚空";
        s.desc = "最强战机，全能型性能。终极之选。";
        s.hp = 6;
        s.speed = 1.5;
        s.viewAtk = 5;
        s.viewDef = 3;
        s.viewRate = 2.0;
        s.viewHp = 6;
        s.cost = 2000;
        break;
    default:
        s.name = "未知";
        s.desc = "未知战机";
        s.hp = 1;
        s.speed = 1.0;
        s.viewAtk = 1;
        s.viewDef = 1;
        s.viewRate = 1.0;
        s.viewHp = 1;
        s.cost = 0;
        break;
    }
    return s;
}

// --- 装备系统实现 ---

QList<int> DataManager::getInventory() { return m_inventory; }

void DataManager::addEquipment(int equipId)
{
    if (!m_inventory.contains(equipId))
    {
        m_inventory.append(equipId);
        saveData();
    }
}

bool DataManager::hasEquipment(int equipId) { return m_inventory.contains(equipId); }

int DataManager::getEquippedId(EquipType type)
{
    switch (type)
    {
    case TYPE_CORE:
        return m_equippedCore;
    case TYPE_ARMOR:
        return m_equippedArmor;
    case TYPE_ENGINE:
        return m_equippedEngine;
    }
    return -1;
}

void DataManager::equipItem(EquipType type, int equipId)
{
    switch (type)
    {
    case TYPE_CORE:
        m_equippedCore = equipId;
        break;
    case TYPE_ARMOR:
        m_equippedArmor = equipId;
        break;
    case TYPE_ENGINE:
        m_equippedEngine = equipId;
        break;
    }
    saveData();
}

// 定义所有装备数据库 (ID规划: 1xx核心, 2xx装甲, 3xx引擎)
Equipment DataManager::getEquipmentById(int id)
{
    Equipment e;
    e.id = id;

    switch (id)
    {
    // --- 核心 (加攻击) ---
    case 101:
        e.type = TYPE_CORE;
        e.tier = TIER_COMMON;
        e.name = "基础火控";
        e.desc = "攻击力+1";
        e.atkBonus = 1;
        e.cost = 500;
        break;
    case 102:
        e.type = TYPE_CORE;
        e.tier = TIER_RARE;
        e.name = "等离子核心";
        e.desc = "攻击力+3";
        e.atkBonus = 3;
        e.cost = 2000;
        break;
    case 103:
        e.type = TYPE_CORE;
        e.tier = TIER_EPIC;
        e.name = "暗物质反应堆";
        e.desc = "攻击力+5";
        e.atkBonus = 5;
        e.cost = 10000;
        break;

    // --- 装甲 (加HP) ---
    case 201:
        e.type = TYPE_ARMOR;
        e.tier = TIER_COMMON;
        e.name = "铁板装甲";
        e.desc = "HP+20";
        e.hpBonus = 20;
        e.cost = 500;
        break;
    case 202:
        e.type = TYPE_ARMOR;
        e.tier = TIER_RARE;
        e.name = "纳米涂层";
        e.desc = "HP+50";
        e.hpBonus = 50;
        e.cost = 2000;
        break;
    case 203:
        e.type = TYPE_ARMOR;
        e.tier = TIER_EPIC;
        e.name = "力场发生器";
        e.desc = "HP+100";
        e.hpBonus = 100;
        e.cost = 10000;
        break;

    // --- 引擎 (加移速/射速) ---
    case 301:
        e.type = TYPE_ENGINE;
        e.tier = TIER_COMMON;
        e.name = "燃烧推进器";
        e.desc = "射速+10%";
        e.rateBonus = 0.1;
        e.cost = 500;
        break;
    case 302:
        e.type = TYPE_ENGINE;
        e.tier = TIER_RARE;
        e.name = "离子引擎";
        e.desc = "射速+25%, 移速+10%";
        e.rateBonus = 0.25;
        e.spdBonus = 0.1;
        e.cost = 2000;
        break;
    case 303:
        e.type = TYPE_ENGINE;
        e.tier = TIER_EPIC;
        e.name = "曲率引擎";
        e.desc = "射速+50%, 移速+20%";
        e.rateBonus = 0.50;
        e.spdBonus = 0.2;
        e.cost = 10000;
        break;

    default:
        e.name = "未知装备";
        break;
    }
    return e;
}

// 掉落逻辑
int DataManager::generateDrop(int bossId)
{
    int roll = QRandomGenerator::global()->bounded(100);
    int tier = TIER_COMMON;

    // 概率：普通70%，优良25%，极品5% (随关卡提升概率)
    if (roll < 5 + bossId * 5)
        tier = TIER_EPIC;
    else if (roll < 30 + bossId * 10)
        tier = TIER_RARE;
    else
        tier = TIER_COMMON;

    // 根据 Boss ID 倾向掉落不同部位
    // Boss 1,4 -> 引擎; Boss 2,5 -> 装甲; Boss 3,6 -> 核心
    int baseId = 0;
    if (bossId == 1 || bossId == 4)
        baseId = 300;
    else if (bossId == 2 || bossId == 5)
        baseId = 200;
    else
        baseId = 100;

    return baseId + 1 + tier; // 例如 301, 302, 303
}

// 计算最终属性
PlaneStats DataManager::getFinalStats(int planeId)
{
    PlaneStats base = getPlaneStats(planeId);

    // 叠加装备属性
    QList<int> equips = {m_equippedCore, m_equippedArmor, m_equippedEngine};
    for (int id : equips)
    {
        if (id == -1)
            continue;
        Equipment e = getEquipmentById(id);
        base.hp += e.hpBonus;
        base.viewAtk += e.atkBonus;
        base.speed *= (1.0 + e.spdBonus);
        base.viewRate *= (1.0 + e.rateBonus); // 射速乘算
    }
    return base;
}