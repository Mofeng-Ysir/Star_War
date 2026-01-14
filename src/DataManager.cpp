#include "DataManager.h"
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>

// 静态成员初始化
int DataManager::m_coins = 0;
int DataManager::m_currentPlaneId = 0;
QList<bool> DataManager::m_unlockedPlanes = {true, false, false, false, false};

QString DataManager::getFilePath()
{
    return QCoreApplication::applicationDirPath() + "/save.dat";
}

void DataManager::loadData()
{
    QFile file(getFilePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        QString line = in.readLine();
        QStringList parts = line.split("|");
        if (parts.size() >= 3)
        {
            m_coins = parts[0].toInt();
            m_currentPlaneId = parts[1].toInt();
            QString mask = parts[2];
            for (int i = 0; i < 5 && i < mask.length(); ++i)
            {
                m_unlockedPlanes[i] = (mask[i] == '1');
            }
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
        out << m_coins << "|" << m_currentPlaneId << "|" << mask;
        file.close();
    }
}

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

// 【核心修改】数值同步
PlaneStats DataManager::getPlaneStats(int id)
{
    PlaneStats s;
    s.id = id;
    switch (id)
    {
    case 0: // 1号: 初始机
        s.name = "勇者号";
        s.cost = 0;
        s.viewAtk = 1.0;
        s.viewDef = 1.0;
        s.viewRate = 1.0;
        s.viewHp = 1.0;
        s.hp = 10;     // 实际血量 = 1 * 10
        s.speed = 1.0; // 移动速度标准
        s.desc = "标准型战机，各项指标均衡。";
        break;

    case 1: // 2号: 双子星
        s.name = "双子星";
        s.cost = 1000;
        s.viewAtk = 3.0;
        s.viewDef = 2.0;
        s.viewRate = 3.0;
        s.viewHp = 3.0;
        s.hp = 30;     // 实际血量 = 3 * 10
        s.speed = 1.2; // 速度稍快
        s.desc = "高射速双发战机，火力压制。";
        break;

    case 2: // 3号: 泰坦
        s.name = "泰坦";
        s.cost = 3000;
        s.viewAtk = 4.0;
        s.viewDef = 5.0;
        s.viewRate = 2.0;
        s.viewHp = 6.0;
        s.hp = 60;     // 实际血量 = 6 * 10
        s.speed = 0.7; // 速度慢，笨重
        s.desc = "重装甲霰弹战机，防御力极强。";
        break;

    case 3: // 4号: 幻影
        s.name = "幻影";
        s.cost = 6000;
        s.viewAtk = 4.0;
        s.viewDef = 4.0;
        s.viewRate = 2.0;
        s.viewHp = 5.0;
        s.hp = 50;     // 实际血量 = 5 * 10
        s.speed = 1.3; // 速度快
        s.desc = "搭载穿透弹与护盾系统。";
        break;

    case 4: // 5号: 虚空
        s.name = "虚空";
        s.cost = 10000;
        s.viewAtk = 5.0;
        s.viewDef = 4.0;
        s.viewRate = 2.5;
        s.viewHp = 8.0;
        s.hp = 80;     // 实际血量 = 8 * 10
        s.speed = 1.0; // 标准速度
        s.desc = "搭载高伤追踪导弹与时空科技。";
        break;

    default:
        s.name = "未知";
        s.cost = 99999;
        break;
    }
    return s;
}