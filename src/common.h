#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QRect>

// --- BOSS 行为状态机 ---
enum BossState
{
    STATE_NORMAL = 0, // 正常阶段：随机移动 + 普通弹幕
    STATE_WARNING,    // 预警阶段：停止移动，显示红线/提示
    STATE_SKILL_DASH, // 技能释放：冲刺 (Boss 1)
    STATE_SKILL_FIRE, // 技能释放：特殊开火 (Boss 2/3...)
    STATE_RECOVERY    // 恢复阶段：原地僵直 (输出机会)
};

// 飞机类型枚举
enum PlaneType
{
    PLANE_DEFAULT = 0,
    PLANE_DOUBLE = 1,
    PLANE_SHOTGUN = 2,
    PLANE_SNIPER = 3,
    PLANE_ALIEN = 4
};

// 飞机属性
struct PlaneStats
{
    int id;
    QString name;
    QString desc;
    int cost;
    int hp;
    double speed;
    double viewAtk;
    double viewDef;
    double viewRate;
    double viewHp;
};

// 子弹
struct Bullet
{
    double x, y;
    double speedX, speedY;
    bool isEnemy;
    bool active;
    int hitCount = 0;
    bool isSpecial = false; // 【新增】是否为特殊技能弹幕 (绘制不同外观)
};

// 敌人
struct Enemy
{
    int type;
    double x, y;
    int hp;
    int maxHp;
    bool active;

    // 计数器
    int shootTimer;
    int skillTimer; // 【新增】技能流程计时器

    // AI 状态
    BossState state; // 【新增】当前状态
    double moveAngle;
    int bossId;

    // 技能参数
    bool isWarning;
    QRect warningRect;
    double attackTargetX; // 锁定目标X
    double attackTargetY; // 锁定目标Y
    double dashSpeedX;    // 【新增】冲刺速度X
    double dashSpeedY;    // 【新增】冲刺速度Y
};

// --- 【新增】装备系统定义 ---

enum EquipTier
{
    TIER_COMMON = 0, // 普通 (白色)
    TIER_RARE = 1,   // 优良 (蓝色)
    TIER_EPIC = 2    // 极品 (紫色)
};

enum EquipType
{
    TYPE_CORE = 0,  // 核心 (加攻击)
    TYPE_ARMOR = 1, // 装甲 (加血量/防御)
    TYPE_ENGINE = 2 // 引擎 (加射速/移速)
};

struct Equipment
{
    int id;         // 唯一ID
    QString name;   // 名称
    EquipType type; // 部位
    EquipTier tier; // 品质
    int cost;       // 商店售价

    // 增益属性
    int hpBonus;      // 血量加成
    double atkBonus;  // 攻击力加成 (子弹伤害)
    double spdBonus;  // 移速加成
    double rateBonus; // 射速加成 (百分比，如 0.1 代表射速快10%)

    QString desc; // 描述
};

// 关卡配置
struct LevelConfig
{
    int levelId;
    int totalWaves;
    int enemyHpScale;
    int bossHp;
};

#endif // COMMON_H