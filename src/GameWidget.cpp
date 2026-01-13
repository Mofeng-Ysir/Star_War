#include "GameWidget.h"
#include "ScoreManager.h"
#include "LevelManager.h"
#include "CollisionSystem.h"
#include "DataManager.h"
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>
#include <QtMath>
#include <QLinearGradient>
#include <QRadialGradient>

// ================= 构造函数 =================
GameWidget::GameWidget(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    loadAssets();

    shootSfx = new QSoundEffect(this);
    shootSfx->setSource(QUrl::fromLocalFile("assets/shoot.wav"));
    shootSfx->setVolume(0.3);

    explodeSfx = new QSoundEffect(this);
    explodeSfx->setSource(QUrl::fromLocalFile("assets/explode.wav"));
    explodeSfx->setVolume(0.6);

    ultSfx = new QSoundEffect(this);
    if (QFileInfo::exists("assets/laser.wav"))
        ultSfx->setSource(QUrl::fromLocalFile("assets/laser.wav"));
    else
        ultSfx->setSource(QUrl::fromLocalFile("assets/shoot.wav"));
    ultSfx->setVolume(1.0);

    bgmPlayer = new QMediaPlayer(this);
    bgmOutput = new QAudioOutput(this);
    bgmPlayer->setAudioOutput(bgmOutput);
    bgmOutput->setVolume(0.3);

    bossMovie = new QMovie(this);
    bossMovie->setCacheMode(QMovie::CacheAll);

    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &GameWidget::updateGame);

    enemySpawnTimer = 0;
    isShieldActive = false;
    isTimeFrozen = false;
    isUltActive = false;
    nukeFlashOpacity = 0;
}

void GameWidget::loadAssets()
{
    imgHero.load("assets/hero.png");
    imgEnemy1.load("assets/enemy.png");
    imgEnemy2.load("assets/enemy2.png");
    imgEnemy3.load("assets/enemy3.png");
    imgBg.load("assets/game_bg.png");

    if (!imgHero.isNull())
        imgHero = imgHero.scaled(60, 60, Qt::KeepAspectRatio);
    if (!imgEnemy1.isNull())
        imgEnemy1 = imgEnemy1.scaled(50, 50, Qt::KeepAspectRatio);
    if (!imgEnemy2.isNull())
        imgEnemy2 = imgEnemy2.scaled(50, 50, Qt::KeepAspectRatio);
    if (!imgEnemy3.isNull())
        imgEnemy3 = imgEnemy3.scaled(120, 120, Qt::KeepAspectRatio);

    bulletImages.clear();
    for (int i = 0; i < 5; ++i)
    {
        QString path = QString("assets/bullet%1.png").arg(i);
        QImage img;
        if (img.load(path))
            img = img.scaled(20, 40, Qt::KeepAspectRatio);
        bulletImages.append(img);
    }
}

// ================= 分辨率适配 (核心修改) =================
double GameWidget::getGameScale()
{
    // 以高度为基准进行缩放，保证垂直视野一致
    return height() / (double)LOGICAL_HEIGHT;
}

double GameWidget::getGameWidth()
{
    // 逻辑宽度 = 物理宽度 / 缩放比
    return width() / getGameScale();
}

QPointF GameWidget::mapToGame(const QPoint &pos)
{
    double scale = getGameScale();
    // 只需要除以缩放比，不需要减去偏移（因为我们现在从(0,0)开始画）
    return QPointF(pos.x() / scale, pos.y() / scale);
}

// ================= 游戏开始 =================
void GameWidget::startGame(int level)
{
    setCursor(Qt::BlankCursor);
    currentLevelConfig = LevelManager::getLevelConfig(level);

    currentPlaneId = DataManager::getCurrentPlaneId();
    PlaneStats stats = DataManager::getPlaneStats(currentPlaneId);

    // 英雄初始位置居中
    heroX = getGameWidth() / 2 - 30;
    heroY = LOGICAL_HEIGHT - 100;
    score = 0;
    heroHp = stats.hp;

    QString heroPath;
    if (currentPlaneId == 0)
        heroPath = "assets/hero.png";
    else
        heroPath = QString("assets/plane%1.png").arg(currentPlaneId);

    if (!imgHero.load(heroPath))
        imgHero.load("assets/hero.png");

    if (currentPlaneId == 2)
        imgHero = imgHero.scaled(80, 80, Qt::KeepAspectRatio);
    else
        imgHero = imgHero.scaled(60, 60, Qt::KeepAspectRatio);

    QString bossIconPath = QString("assets/boss%1.png").arg(level);
    if (!QFileInfo::exists(bossIconPath))
        bossIconPath = QString("assets/boss%1.gif").arg(level);
    if (!currentBossIcon.load(bossIconPath))
        currentBossIcon = imgEnemy3;
    currentBossIcon = currentBossIcon.scaled(60, 60, Qt::KeepAspectRatio);

    isGameOver = false;
    isVictory = false;
    heroShootTimer = 0;
    progressCounter = 0;
    bossSpawned = false;
    enemySpawnTimer = 0;

    isUltActive = false;
    ultDurationTimer = 0;
    ULT_COOLDOWN_MAX = 500;
    ultCooldownTimer = ULT_COOLDOWN_MAX;

    isShieldActive = false;
    isTimeFrozen = false;
    nukeFlashOpacity = 0;

    bossStrategy.reset();

    bullets.clear();
    enemies.clear();

    if (bossMovie->isValid())
        bossMovie->stop();

    QString bgmPath = "assets/game_bgm.mp3";
    bgmPlayer->setSource(QUrl::fromLocalFile(bgmPath));
    bgmPlayer->setLoops(QMediaPlayer::Infinite);
    bgmPlayer->play();

    gameTimer->start(16);
}

void GameWidget::stopGame()
{
    gameTimer->stop();
    bgmPlayer->stop();
    if (bossMovie->isValid())
        bossMovie->setPaused(true);
    setCursor(Qt::ArrowCursor);
}

void GameWidget::fireUlt()
{
    if (isGameOver || isVictory)
        return;

    if (ultCooldownTimer >= ULT_COOLDOWN_MAX && !isUltActive && !isShieldActive && !isTimeFrozen)
    {
        ultCooldownTimer = 0;
        ultSfx->play();

        switch (currentPlaneId)
        {
        case PLANE_DEFAULT:
            isUltActive = true;
            ultDurationTimer = 20;
            break;
        case PLANE_DOUBLE:
            for (int i = 0; i < 36; i++)
            {
                Bullet b;
                b.x = heroX + imgHero.width() / 2;
                b.y = heroY;
                b.isEnemy = false;
                b.active = true;
                double rad = qDegreesToRadians(i * 10.0);
                b.speedX = qCos(rad) * 12.0;
                b.speedY = qSin(rad) * 12.0;
                bullets.append(b);
            }
            break;
        case PLANE_SHOTGUN:
            for (auto &b : bullets)
                if (b.isEnemy)
                    b.active = false;
            for (auto &e : enemies)
            {
                if (e.active)
                {
                    e.hp -= 50;
                    if (e.hp <= 0)
                    {
                        e.active = false;
                        int add = (e.type == 10) ? (500 * currentLevelConfig.levelId) : 100;
                        score += add;
                        if (e.type == 10 && bossMovie->isValid())
                            bossMovie->setPaused(true);
                        if (e.type != 10 && progressCounter < currentLevelConfig.totalWaves)
                            progressCounter++;
                    }
                }
            }
            nukeFlashOpacity = 255;
            explodeSfx->play();
            break;
        case PLANE_SNIPER:
            isShieldActive = true;
            shieldTimer = 300;
            break;
        case PLANE_ALIEN:
            isTimeFrozen = true;
            freezeTimer = 300;
            break;
        }
    }
}

// ================= 游戏更新 =================
void GameWidget::updateGame()
{
    if (isGameOver || isVictory)
        return;

    if (nukeFlashOpacity > 0)
    {
        nukeFlashOpacity -= 10;
        if (nukeFlashOpacity < 0)
            nukeFlashOpacity = 0;
    }

    // 1. 英雄普攻
    heroShootTimer++;
    int shootInterval = 12;
    if (currentPlaneId == PLANE_SNIPER)
        shootInterval = 12;

    if (heroShootTimer > shootInterval)
    {
        heroShootTimer = 0;
        if (currentPlaneId == PLANE_DEFAULT || currentPlaneId == PLANE_SNIPER || currentPlaneId == PLANE_ALIEN)
        {
            Bullet b;
            b.x = heroX + imgHero.width() / 2 - 4;
            b.y = heroY;
            b.speedX = 0;
            b.speedY = -12.0;
            b.isEnemy = false;
            b.active = true;
            bullets.append(b);
        }
        else if (currentPlaneId == PLANE_DOUBLE)
        {
            for (int k : {-10, 10})
            {
                Bullet b;
                b.x = heroX + imgHero.width() / 2 + k;
                b.y = heroY;
                b.speedX = 0;
                b.speedY = -12.0;
                b.isEnemy = false;
                b.active = true;
                bullets.append(b);
            }
        }
        else if (currentPlaneId == PLANE_SHOTGUN)
        {
            for (int k : {-1, 0, 1})
            {
                Bullet b;
                b.x = heroX + imgHero.width() / 2;
                b.y = heroY;
                b.speedX = k * 3.0;
                b.speedY = -10.0;
                b.isEnemy = false;
                b.active = true;
                bullets.append(b);
            }
        }
        shootSfx->play();
    }

    // 2. 技能状态
    if (isUltActive)
    {
        ultDurationTimer--;
        if (ultDurationTimer <= 0)
            isUltActive = false;
    }
    else if (isShieldActive)
    {
        shieldTimer--;
        if (shieldTimer <= 0)
            isShieldActive = false;
    }
    else if (isTimeFrozen)
    {
        freezeTimer--;
        if (freezeTimer <= 0)
            isTimeFrozen = false;
    }
    else
    {
        if (ultCooldownTimer < ULT_COOLDOWN_MAX)
            ultCooldownTimer++;
    }

    // 3. 刷怪
    if (!isTimeFrozen)
    {
        if (!bossSpawned)
        {
            if (progressCounter < currentLevelConfig.totalWaves)
                spawnEnemy();
            else
                spawnBoss();
        }
    }

    // 4. 敌人更新
    for (auto &e : enemies)
    {
        if (isTimeFrozen)
            continue;

        if (e.type == 10)
        {
            // BOSS 策略 (使用动态宽度)
            bossStrategy.update(e, bullets, heroX, heroY, (int)getGameWidth(), LOGICAL_HEIGHT, currentLevelConfig);
        }
        else
        {
            e.y += 3.0;
            if (e.type == 1)
            {
                e.shootTimer++;
                if (e.shootTimer > 80)
                {
                    e.shootTimer = 0;
                    Bullet b;
                    b.x = e.x + 25;
                    b.y = e.y + 50;
                    b.speedX = 0;
                    b.speedY = 7.0;
                    b.isEnemy = true;
                    b.active = true;
                    bullets.append(b);
                }
            }
            if (e.y > LOGICAL_HEIGHT)
                e.active = false;
        }
    }

    // 5. 子弹更新
    for (auto &b : bullets)
    {
        if (isTimeFrozen && b.isEnemy)
            continue;

        if (!b.isEnemy && currentPlaneId == PLANE_ALIEN && !enemies.isEmpty())
        {
            Enemy *closest = nullptr;
            double minDist = 100000;
            for (auto &e : enemies)
            {
                if (!e.active)
                    continue;
                if (e.type == 10)
                {
                    closest = &e;
                    break;
                }
                double d = qSqrt(qPow(e.x - b.x, 2) + qPow(e.y - b.y, 2));
                if (d < minDist)
                {
                    minDist = d;
                    closest = &e;
                }
            }
            if (closest)
            {
                double targetX = closest->x + 25;
                double targetY = closest->y + 25;
                double dx = targetX - b.x;
                double dy = targetY - b.y;
                double angle = qAtan2(dy, dx);
                b.speedX = qCos(angle) * 15.0;
                b.speedY = qSin(angle) * 15.0;
            }
        }

        b.x += b.speedX;
        b.y += b.speedY;
        // 边界销毁 (使用动态宽度)
        if (b.y < -20 || b.y > LOGICAL_HEIGHT + 20 || b.x < -20 || b.x > getGameWidth() + 20)
            b.active = false;
    }

    checkCollisions();

    if (bossSpawned && enemies.isEmpty())
        victory();
    cleanUp();
    update();
}

void GameWidget::checkCollisions()
{
    auto result = CollisionSystem::check(
        heroX, heroY, imgHero.width(), imgHero.height(),
        bullets, enemies,
        (currentPlaneId == PLANE_DEFAULT && isUltActive),
        isShieldActive,
        currentPlaneId,
        currentLevelConfig.levelId, currentLevelConfig.totalWaves,
        progressCounter, imgEnemy1, imgEnemy3);

    score += result.scoreAdded;
    if (result.scoreAdded > 0)
        explodeSfx->play();

    if (result.bossDied)
    {
        if (bossMovie->isValid())
            bossMovie->setPaused(true);
    }

    if (result.heroHit)
    {
        if (!isShieldActive)
        {
            heroHp -= result.heroDamageTaken;
            if (heroHp <= 0)
                gameOver();
        }
    }
}

// 刷怪 (使用动态宽度)
void GameWidget::spawnEnemy()
{
    enemySpawnTimer++;
    int spawnRate = 60 - (currentLevelConfig.levelId * 5);
    if (spawnRate < 20)
        spawnRate = 20;

    if (enemySpawnTimer > spawnRate)
    {
        enemySpawnTimer = 0;
        Enemy e;
        e.x = QRandomGenerator::global()->bounded((int)getGameWidth() - 50);
        e.y = -50;
        e.active = true;
        e.shootTimer = 0;
        e.moveAngle = 0;
        e.bossId = 0;

        int r = QRandomGenerator::global()->bounded(100);
        if (r < 60)
        {
            e.type = 0;
            e.hp = 1 * currentLevelConfig.enemyHpScale;
            e.maxHp = e.hp;
        }
        else if (r < 90)
        {
            e.type = 1;
            e.hp = 2 * currentLevelConfig.enemyHpScale;
            e.maxHp = e.hp;
        }
        else
        {
            e.type = 2;
            e.hp = 5 * currentLevelConfig.enemyHpScale;
            e.maxHp = e.hp;
        }

        enemies.append(e);
    }
}

// 生成BOSS (使用动态宽度居中)
void GameWidget::spawnBoss()
{
    if (bossSpawned)
        return;
    bossSpawned = true;

    QString bossBgmPath = QString("assets/boss%1_bgm.mp3").arg(currentLevelConfig.levelId);
    if (!QFileInfo::exists(bossBgmPath))
        bossBgmPath = "assets/game_bgm.mp3";
    bgmPlayer->stop();
    bgmPlayer->setSource(QUrl::fromLocalFile(bossBgmPath));
    bgmPlayer->play();

    QString bossGifPath = QString("assets/boss%1.gif").arg(currentLevelConfig.levelId);
    if (QFileInfo::exists(bossGifPath))
    {
        bossMovie->setFileName(bossGifPath);
        bossMovie->start();
    }
    else
    {
        bossMovie->setFileName("");
    }

    Enemy boss;
    boss.type = 10;
    boss.x = getGameWidth() / 2 - 100; // 动态居中
    boss.y = -150;
    boss.active = true;
    boss.maxHp = currentLevelConfig.bossHp;
    boss.hp = boss.maxHp;
    boss.shootTimer = 0;
    boss.moveAngle = 0;
    boss.bossId = currentLevelConfig.levelId;
    enemies.append(boss);
}

// ================= 绘制 (核心修改: 铺满全屏) =================
void GameWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    // 1. 绘制背景 (直接铺满窗口，无视比例)
    if (!imgBg.isNull())
        p.drawImage(rect(), imgBg);
    else
        p.fillRect(rect(), Qt::black);

    // 2. 坐标变换：只缩放，不平移(X轴)
    // 这样游戏内容会铺满全屏，但高度固定为 600 (逻辑单位)
    double scale = getGameScale();
    double gameW = getGameWidth();

    p.save();
    p.scale(scale, scale); // 只缩放
    // 此时 p 的坐标系：(0,0) 就是左上角，宽度是 gameW，高度是 600

    // 3.1 时空冻结
    if (isTimeFrozen)
        p.fillRect(QRect(0, 0, (int)gameW, LOGICAL_HEIGHT), QColor(0, 0, 255, 50));

    // 3.2 激光
    if (currentPlaneId == PLANE_DEFAULT && isUltActive)
    {
        QLinearGradient gradient(heroX + imgHero.width() / 2 - 40, 0, heroX + imgHero.width() / 2 + 40, 0);
        gradient.setColorAt(0.0, QColor(0, 255, 255, 0));
        gradient.setColorAt(0.5, QColor(255, 255, 255, 255));
        gradient.setColorAt(1.0, QColor(0, 255, 255, 0));
        p.setBrush(gradient);
        p.setPen(Qt::NoPen);
        p.drawRect(heroX + imgHero.width() / 2 - 40, 0, 80, heroY);
    }

    // 3.3 英雄
    p.drawImage(heroX, heroY, imgHero);

    if (isShieldActive)
    {
        p.setPen(QPen(Qt::cyan, 3));
        p.setBrush(QColor(0, 255, 255, 50));
        p.drawEllipse(QPointF(heroX + imgHero.width() / 2, heroY + imgHero.height() / 2), 50, 50);
    }

    // 3.4 敌人
    for (const auto &e : enemies)
    {
        if (e.type == 10)
        {
            if (bossMovie->isValid() && (bossMovie->state() == QMovie::Running || bossMovie->state() == QMovie::Paused))
            {
                p.drawPixmap(QRect(e.x, e.y, 200, 150), bossMovie->currentPixmap());
            }
            else
            {
                p.drawImage(e.x, e.y, imgEnemy3);
            }
            p.setBrush(Qt::red);
            p.drawRect(e.x, e.y - 15, 200, 8);
            p.setBrush(Qt::green);
            float hpPer = (float)e.hp / (float)e.maxHp;
            p.drawRect(e.x, e.y - 15, 200 * hpPer, 8);
        }
        else
        {
            QImage *currentImg = &imgEnemy1;
            if (e.type == 1)
                currentImg = &imgEnemy2;
            if (e.type == 2)
                currentImg = &imgEnemy3;
            p.drawImage(e.x, e.y, *currentImg);
        }
    }

    // 3.5 子弹
    p.setPen(Qt::NoPen);
    for (const auto &b : bullets)
    {
        if (!b.active)
            continue;

        if (b.isEnemy)
        {
            QRadialGradient gradient(b.x + 4, b.y + 4, 6);
            gradient.setColorAt(0.0, QColor(255, 255, 255));
            gradient.setColorAt(0.5, QColor(255, 0, 0));
            gradient.setColorAt(1.0, QColor(100, 0, 0, 0));
            p.setBrush(gradient);
            p.drawEllipse(b.x, b.y, 10, 10);
        }
        else
        {
            bool hasImage = false;
            if (currentPlaneId < bulletImages.size() && !bulletImages[currentPlaneId].isNull())
            {
                p.drawImage(b.x - 5, b.y, bulletImages[currentPlaneId]);
                hasImage = true;
            }
            if (!hasImage)
            {
                switch (currentPlaneId)
                {
                case PLANE_DEFAULT:
                {
                    QLinearGradient g(b.x, b.y, b.x, b.y + 15);
                    g.setColorAt(0, QColor(255, 255, 200));
                    g.setColorAt(1, QColor(255, 165, 0));
                    p.setBrush(g);
                    p.drawRect(b.x + 2, b.y, 4, 14);
                    break;
                }
                case PLANE_DOUBLE:
                {
                    QRadialGradient g(b.x + 4, b.y + 6, 8);
                    g.setColorAt(0, Qt::white);
                    g.setColorAt(0.6, QColor(0, 200, 255));
                    g.setColorAt(1, QColor(0, 0, 255, 0));
                    p.setBrush(g);
                    p.drawEllipse(b.x, b.y, 8, 12);
                    break;
                }
                case PLANE_SHOTGUN:
                {
                    QRadialGradient g(b.x + 5, b.y + 5, 6);
                    g.setColorAt(0, Qt::white);
                    g.setColorAt(0.5, QColor(255, 50, 0));
                    g.setColorAt(1, QColor(100, 0, 0, 0));
                    p.setBrush(g);
                    p.drawEllipse(b.x, b.y, 10, 10);
                    break;
                }
                case PLANE_SNIPER:
                {
                    p.setBrush(QColor(200, 0, 255, 100));
                    p.drawRect(b.x + 1, b.y - 5, 6, 25);
                    p.setBrush(Qt::white);
                    p.drawRect(b.x + 3, b.y, 2, 20);
                    break;
                }
                case PLANE_ALIEN:
                {
                    QRadialGradient g(b.x + 4, b.y + 4, 6);
                    g.setColorAt(0, QColor(200, 255, 200));
                    g.setColorAt(0.5, QColor(0, 255, 0));
                    g.setColorAt(1, QColor(0, 50, 0, 0));
                    p.setBrush(g);
                    p.drawEllipse(b.x, b.y, 8, 8);
                    break;
                }
                }
            }
        }
    }

    // UI - 核弹闪白
    if (nukeFlashOpacity > 0)
    {
        p.fillRect(QRect(0, 0, (int)gameW, LOGICAL_HEIGHT), QColor(255, 255, 255, nukeFlashOpacity));
    }

    p.restore(); // === 恢复坐标系，绘制 UI ===

    // 4. UI 绘制 (不缩放，贴边)

    // 左上角信息
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 16, QFont::Bold));
    int textMargin = 20;
    p.drawText(textMargin, 40, "Level " + QString::number(currentLevelConfig.levelId));
    p.drawText(textMargin, 70, "Score: " + QString::number(score));

    p.drawText(textMargin, 100, "HP:");
    p.setBrush(Qt::gray);
    p.drawRect(textMargin + 40, 85, 200, 15);
    PlaneStats s = DataManager::getPlaneStats(currentPlaneId);
    float hpRatio = (float)heroHp / s.hp;
    if (hpRatio < 0)
        hpRatio = 0;
    if (hpRatio > 0.3)
        p.setBrush(QColor(0, 255, 255));
    else
        p.setBrush(Qt::red);
    p.drawRect(textMargin + 40, 85, 200 * hpRatio, 15);

    drawProgressBar(p);
    drawUltUI(p);

    if (isGameOver)
    {
        p.setPen(Qt::red);
        p.setFont(QFont("Microsoft YaHei", 40, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "GAME OVER");
    }
    else if (isVictory)
    {
        p.setPen(Qt::yellow);
        p.setFont(QFont("Microsoft YaHei", 40, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "VICTORY!\nLevel Complete");
        p.setFont(QFont("Arial", 20));
        p.drawText(rect().adjusted(0, 150, 0, 0), Qt::AlignCenter, "Click to Continue");
    }
}

// UI 绘制辅助
void GameWidget::drawProgressBar(QPainter &p)
{
    int barWidth = 12;
    int barHeight = 180;
    int marginX = 20;
    int marginY = 60;

    // 右上角位置 (使用真实宽度)
    int barX = width() - barWidth - marginX;
    int barY = marginY;

    p.setPen(QPen(QColor(0, 255, 255, 100), 1));
    p.setBrush(QColor(0, 20, 40, 200));
    p.drawRect(barX, barY, barWidth, barHeight);

    float percentage = 0;
    if (currentLevelConfig.totalWaves > 0)
        percentage = (float)progressCounter / (float)currentLevelConfig.totalWaves;
    if (percentage > 1.0f)
        percentage = 1.0f;

    if (percentage > 0)
    {
        int fillHeight = (int)((barHeight - 2) * percentage);
        p.setPen(Qt::NoPen);
        QLinearGradient gradient(barX, barY + barHeight, barX, barY);
        gradient.setColorAt(0.0, QColor(0, 100, 200));
        gradient.setColorAt(1.0, QColor(0, 255, 255));
        p.setBrush(gradient);
        p.drawRect(barX + 1, barY + barHeight - fillHeight - 1, barWidth - 2, fillHeight);
    }

    // 【修改】移除外框，只有图片
    int iconSize = 50;
    int iconX = barX + barWidth / 2 - iconSize / 2 - 30;
    int iconY = barY - iconSize - 5;

    p.drawImage(iconX, iconY, currentBossIcon.scaled(iconSize, iconSize, Qt::KeepAspectRatioByExpanding));

    // 已删除文字提示
}

void GameWidget::drawUltUI(QPainter &p)
{
    int iconSize = 70;
    int x = 30;
    int y = height() - iconSize - 30; // 使用真实高度，贴底

    p.setBrush(QColor(0, 0, 0, 180));
    p.setPen(QPen(Qt::white, 2));
    p.drawEllipse(x, y, iconSize, iconSize);

    QImage icon = imgHero.scaled(iconSize - 20, iconSize - 20, Qt::KeepAspectRatio);
    int imgX = x + (iconSize - icon.width()) / 2;
    int imgY = y + (iconSize - icon.height()) / 2;
    p.drawImage(imgX, imgY, icon);

    if (isShieldActive || isTimeFrozen)
    {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::green, 3));
        p.drawEllipse(x, y, iconSize, iconSize);
        p.setPen(Qt::green);
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(x, y - 20, iconSize, 20, Qt::AlignCenter, "ACTIVE");
        return;
    }

    if (ultCooldownTimer < ULT_COOLDOWN_MAX)
    {
        p.setBrush(QColor(0, 0, 0, 200));
        p.setPen(Qt::NoPen);
        int spanAngle = 360 * 16 * (ULT_COOLDOWN_MAX - ultCooldownTimer) / ULT_COOLDOWN_MAX;
        p.drawPie(x, y, iconSize, iconSize, 90 * 16, spanAngle);
    }
    else
    {
        p.setPen(QPen(QColor(0, 255, 255), 3));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(x, y, iconSize, iconSize);
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.setPen(Qt::cyan);
        p.drawText(x, y - 20, iconSize, 20, Qt::AlignCenter, "READY");
    }
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(x, y + iconSize + 5, iconSize, 20, Qt::AlignCenter, "ULT");
}

// 结算逻辑
void GameWidget::gameOver()
{
    if (!isGameOver && !isVictory)
    {
        isGameOver = true;
        if (bossMovie->isValid())
            bossMovie->setPaused(true);
        setCursor(Qt::ArrowCursor);
        int coinsEarned = score / 10;
        DataManager::addCoins(coinsEarned);
    }
}

void GameWidget::victory()
{
    if (!isVictory && !isGameOver)
    {
        isVictory = true;
        if (bossMovie->isValid())
            bossMovie->setPaused(true);
        setCursor(Qt::ArrowCursor);
        ScoreManager::saveScore(score);
        LevelManager::unlockNextLevel(currentLevelConfig.levelId);
        int coinsEarned = score / 10;
        DataManager::addCoins(coinsEarned);
    }
}

// 输入映射
void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isGameOver || isVictory)
        return;
    QPointF gamePos = mapToGame(event->pos());
    double targetX = gamePos.x() - imgHero.width() / 2;
    double targetY = gamePos.y() - imgHero.height() / 2;
    // 边界限制现在使用 getGameWidth()
    if (targetX < 0)
        targetX = 0;
    if (targetX > getGameWidth() - imgHero.width())
        targetX = getGameWidth() - imgHero.width();
    if (targetY < 0)
        targetY = 0;
    if (targetY > LOGICAL_HEIGHT - imgHero.height())
        targetY = LOGICAL_HEIGHT - imgHero.height();
    heroX = targetX;
    heroY = targetY;
}

void GameWidget::mousePressEvent(QMouseEvent *event)
{
    if (isGameOver || isVictory)
        return;
    if (event->button() == Qt::LeftButton)
    {
        fireUlt();
    }
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (isGameOver)
        emit gameEnded();
    else if (isVictory)
        emit levelWon();
}

void GameWidget::keyPressEvent(QKeyEvent *event) {}

void GameWidget::cleanUp()
{
    QMutableListIterator<Bullet> i(bullets);
    while (i.hasNext())
        if (!i.next().active)
            i.remove();
    QMutableListIterator<Enemy> j(enemies);
    while (j.hasNext())
        if (!j.next().active)
            j.remove();
}