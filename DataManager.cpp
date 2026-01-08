#include "DataManager.h"
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>

// 静态成员初始化
int DataManager::m_coins = 0;
int DataManager::m_currentPlaneId = 0;
QList<bool> DataManager::m_unlockedPlanes = {true, false, false, false, false}; // 默认只解锁第1个

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

PlaneStats DataManager::getPlaneStats(int id)
{
    PlaneStats s;
    s.id = id;
    switch (id)
    {
    case 0:
        s.name = "勇者号";
        s.cost = 0;
        s.hp = 10;
        s.speed = 1.0;
        s.desc = "普攻: 单发\n大招: 毁灭激光";
        break;
    case 1:
        s.name = "双子星";
        s.cost = 1000;
        s.hp = 10;
        s.speed = 1.2;
        s.desc = "普攻: 双管齐下\n大招: 全屏弹幕雨";
        break;
    case 2:
        s.name = "泰坦";
        s.cost = 3000;
        s.hp = 15;
        s.speed = 0.8;
        s.desc = "普攻: 三向霰弹\n大招: 核弹清屏";
        break;
    case 3:
        s.name = "幻影";
        s.cost = 6000;
        s.hp = 8;
        s.speed = 1.5;
        s.desc = "普攻: 极速穿透\n大招: 绝对防御(5秒)";
        break;
    case 4:
        s.name = "虚空";
        s.cost = 10000;
        s.hp = 10;
        s.speed = 1.0;
        s.desc = "普攻: 自动追踪\n大招: 时空冻结";
        break;
    default:
        s.name = "未知";
        s.cost = 99999;
        break;
    }
    return s;
}