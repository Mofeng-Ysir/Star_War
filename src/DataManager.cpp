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

// 【核心修改】这里定义了每一架飞机的具体参数和描述
PlaneStats DataManager::getPlaneStats(int id)
{
    PlaneStats s;
    s.id = id;
    switch (id)
    {
    case 0:
        s.name = "勇者号 (Voyager)";
        s.cost = 0;
        s.hp = 10;
        s.speed = 1.0;
        s.desc = "【新手推荐】\n"
                 "普攻：标准单发光束\n"
                 "大招：<font color='#00FFFF'>毁灭激光</font> (持续高伤，消除弹幕)";
        break;
    case 1:
        s.name = "双子星 (Gemini)";
        s.cost = 500;
        s.hp = 12;
        s.speed = 1.2;
        s.desc = "【火力覆盖】\n"
                 "普攻：双管齐射，射速中等\n"
                 "大招：<font color='#FF00FF'>弹幕风暴</font> (全屏无死角散射)";
        break;
    case 2:
        s.name = "泰坦 (Titan)";
        s.cost = 1500;
        s.hp = 20;
        s.speed = 0.8;
        s.desc = "【重装坦克】\n"
                 "普攻：三向霰弹，近身伤害极高\n"
                 "大招：<font color='#FFAA00'>战术核弹</font> (清屏秒杀，瞬间巨额伤害)";
        break;
    case 3:
        s.name = "幻影 (Phantom)";
        s.cost = 3000;
        s.hp = 8;
        s.speed = 1.5;
        s.desc = "【极速突袭】\n"
                 "普攻：高速穿透弹，射速极快\n"
                 "大招：<font color='#00FF00'>绝对领域</font> (开启5秒无敌护盾)";
        break;
    case 4:
        s.name = "虚空 (Void)";
        s.cost = 8000;
        s.hp = 10;
        s.speed = 1.0;
        s.desc = "【神秘科技】\n"
                 "普攻：自动追踪导弹，必中\n"
                 "大招：<font color='#0000FF'>时空冻结</font> (暂停所有敌人和子弹5秒)";
        break;
    default:
        s.name = "未知机型";
        s.cost = 99999;
        s.desc = "数据丢失...";
        break;
    }
    return s;
}