#include "GameWidget.h"
#include "ScoreManager.h"
#include "LevelManager.h"
#include "CollisionSystem.h"
#include "DataManager.h" // 引入数据管理
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>
#include <QtMath>
#include <QLinearGradient>

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
    // 如果有 laser.wav 就用，没有就用 shoot.wav
    if (QFileInfo::exists("assets/laser.wav"))
        ultSfx->setSource(QUrl::fromLocalFile("assets/laser.wav"));
    else
        ultSfx->setSource(QUrl::fromLocalFile("assets/shoot.wav"));
    ultSfx->setVolume(1.0);

    bgmPlayer = new QMediaPlayer(this);
    bgmOutput = new QAudioOutput(this);
    bgmPlayer->setAudioOutput(bgmOutput);
    bgmOutput->setVolume(0.3);

    bossVideoPlayer = new QMediaPlayer(this);
    bossVideoSink = new QVideoSink(this);
    bossVideoPlayer->setVideoOutput(bossVideoSink);
    QAudioOutput *nullAudio = new QAudioOutput(this);
    nullAudio->setVolume(0);
    bossVideoPlayer->setAudioOutput(nullAudio);

    connect(bossVideoSink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame)
            { currentBossFrame = frame; });

    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &GameWidget::updateGame);

    enemySpawnTimer = 0;

    // 初始化变量
    isShieldActive = false;
    isTimeFrozen = false;
    isUltActive = false;
}

void GameWidget::loadAssets()
{
    imgHero.load("assets/hero.png");
    imgEnemy1.load("assets/enemy.png");
    imgEnemy2.load("assets/enemy2.png");
    imgEnemy3.load("assets/enemy3.png");
    imgBg.load("assets/game_bg.png");
    imgUltIcon.load("assets/enemy2.png"); // 暂时借用图标

    if (!imgHero.isNull())
        imgHero = imgHero.scaled(60, 60, Qt::KeepAspectRatio);
    if (!imgEnemy1.isNull())
        imgEnemy1 = imgEnemy1.scaled(50, 50, Qt::KeepAspectRatio);
    if (!imgEnemy2.isNull())
        imgEnemy2 = imgEnemy2.scaled(50, 50, Qt::KeepAspectRatio);
    if (!imgEnemy3.isNull())
        imgEnemy3 = imgEnemy3.scaled(120, 120, Qt::KeepAspectRatio);
}

void GameWidget::startGame(int level)
{
    setCursor(Qt::BlankCursor);
    currentLevelConfig = LevelManager::getLevelConfig(level);

    // --- 战机初始化 ---
    currentPlaneId = DataManager::getCurrentPlaneId();
    PlaneStats stats = DataManager::getPlaneStats(currentPlaneId);

    heroX = width() / 2 - 30;
    heroY = height() - 100;
    score = 0;
    heroHp = stats.hp; // 使用战机血量

    // --- 修改开始：加载对应的飞机图片 ---
    QString heroPath;
    if (currentPlaneId == 0)
    {
        heroPath = "assets/hero.png";
    }
    else
    {
        heroPath = QString("assets/plane%1.png").arg(currentPlaneId);
    }

    // 尝试加载，如果失败则回退到默认图片
    if (!imgHero.load(heroPath))
    {
        qDebug() << "Failed to load hero image:" << heroPath << "Using default.";
        imgHero.load("assets/hero.png");
    }

    // 统一缩放大小 (或者你可以根据飞机ID设置不同的大小)
    if (currentPlaneId == 2)
    {
        // 比如：泰坦(ID=2)比较大
        imgHero = imgHero.scaled(80, 80, Qt::KeepAspectRatio);
    }
    else
    {
        // 其他飞机正常大小
        imgHero = imgHero.scaled(60, 60, Qt::KeepAspectRatio);
    }
    // --- 修改结束 ---

    // 状态重置
    isGameOver = false;
    isVictory = false;
    heroShootTimer = 0;
    progressCounter = 0;
    bossSpawned = false;
    enemySpawnTimer = 0;

    isUltActive = false;
    ultDurationTimer = 0;
    ULT_COOLDOWN_MAX = 500;              // 默认8秒
    ultCooldownTimer = ULT_COOLDOWN_MAX; // 开局满能量

    isShieldActive = false;
    isTimeFrozen = false;

    bossStrategy.reset();

    bullets.clear();
    enemies.clear();
    bossVideoPlayer->stop();

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
    bossVideoPlayer->stop();
    setCursor(Qt::ArrowCursor);
}

void GameWidget::fireUlt()
{
    if (isGameOver || isVictory)
        return;

    // 只有冷却好且不在释放中才可释放
    if (ultCooldownTimer >= ULT_COOLDOWN_MAX && !isUltActive && !isShieldActive && !isTimeFrozen)
    {

        ultCooldownTimer = 0; // 消耗能量

        switch (currentPlaneId)
        {
        case PLANE_DEFAULT: // 激光
            isUltActive = true;
            ultDurationTimer = 20; // 0.3秒
            ultSfx->play();
            break;

        case PLANE_DOUBLE: // 全屏弹幕雨
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
            ultSfx->play();
            break;

        case PLANE_SHOTGUN: // 核弹清屏
            for (auto &b : bullets)
                if (b.isEnemy)
                    b.active = false; // 消弹
            for (auto &e : enemies)
            {
                if (e.active)
                {
                    e.hp -= 50; // 巨额伤害
                    if (e.hp <= 0)
                    {
                        e.active = false;
                        score += 100;
                        if (e.type == 10)
                            bossVideoPlayer->stop();
                    }
                }
            }
            explodeSfx->play();
            break;

        case PLANE_SNIPER: // 绝对防御
            isShieldActive = true;
            shieldTimer = 300; // 5秒
            break;

        case PLANE_ALIEN: // 时空冻结
            isTimeFrozen = true;
            freezeTimer = 300; // 5秒
            break;
        }
    }
}

void GameWidget::updateGame()
{
    if (isGameOver || isVictory)
        return;

    // --- 1. 英雄普攻 (根据飞机类型) ---
    heroShootTimer++;
    int shootInterval = 12;
    if (currentPlaneId == PLANE_SNIPER)
        shootInterval = 8; // 幻影射速快

    if (heroShootTimer > shootInterval)
    {
        heroShootTimer = 0;

        if (currentPlaneId == PLANE_DEFAULT || currentPlaneId == PLANE_SNIPER || currentPlaneId == PLANE_ALIEN)
        {
            // 单发
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
            // 双发
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
            // 三向散弹
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

    // --- 2. 技能状态更新 ---
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
        // 只有非技能状态才回蓝
        if (ultCooldownTimer < ULT_COOLDOWN_MAX)
            ultCooldownTimer++;
    }

    // --- 3. 刷怪逻辑 (冻结时不刷怪) ---
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

    // --- 4. 敌人更新 ---
    for (auto &e : enemies)
    {
        if (isTimeFrozen)
            continue; // 冻结：敌人不动

        if (e.type == 10)
        {
            bossStrategy.update(e, bullets, heroX, heroY, width(), height(), currentLevelConfig);
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
            if (e.y > height())
                e.active = false;
        }
    }

    // --- 5. 子弹更新 ---
    for (auto &b : bullets)
    {
        // 冻结：敌人子弹悬停，我方子弹继续飞
        if (isTimeFrozen && b.isEnemy)
            continue;

        // 虚空号：子弹追踪最近敌人
        if (!b.isEnemy && currentPlaneId == PLANE_ALIEN && !enemies.isEmpty())
        {
            Enemy *closest = nullptr;
            double minDist = 100000;
            for (auto &e : enemies)
            {
                if (!e.active)
                    continue;
                double d = qSqrt(qPow(e.x - b.x, 2) + qPow(e.y - b.y, 2));
                if (d < minDist)
                {
                    minDist = d;
                    closest = &e;
                }
            }
            if (closest)
            {
                // 简单的引导逻辑
                if (closest->x > b.x)
                    b.speedX += 0.5;
                else
                    b.speedX -= 0.5;
                // 限制最大横向速度
                if (b.speedX > 5)
                    b.speedX = 5;
                if (b.speedX < -5)
                    b.speedX = -5;
            }
        }

        b.x += b.speedX;
        b.y += b.speedY;
        if (b.y < -20 || b.y > height() + 20 || b.x < -20 || b.x > width() + 20)
            b.active = false;
    }

    // 6. 碰撞检测
    checkCollisions();

    if (bossSpawned && enemies.isEmpty())
    {
        victory();
    }

    cleanUp();
    update();
}

void GameWidget::checkCollisions()
{
    // 委托给 CollisionSystem
    // 注意：如果是 ShieldActive，我们忽略受击逻辑

    auto result = CollisionSystem::check(
        heroX, heroY, imgHero.width(), imgHero.height(),
        bullets, enemies,
        (currentPlaneId == PLANE_DEFAULT && isUltActive), // 只有默认飞机的激光才在这里传 true
        currentLevelConfig.levelId, currentLevelConfig.totalWaves,
        progressCounter, imgEnemy1, imgEnemy3);

    score += result.scoreAdded;
    if (result.scoreAdded > 0)
        explodeSfx->play();
    if (result.bossDied)
        bossVideoPlayer->stop();

    if (result.heroHit)
    {
        if (!isShieldActive)
        { // 有护盾不扣血
            heroHp -= result.heroDamageTaken;
            if (heroHp <= 0)
                gameOver();
        }
    }
}

void GameWidget::drawUltUI(QPainter &p)
{
    int iconSize = 60;
    int x = 20;
    int y = height() - iconSize - 20;

    p.setBrush(QColor(0, 0, 0, 180));
    p.setPen(QPen(Qt::white, 2));
    p.drawEllipse(x, y, iconSize, iconSize);
    p.drawImage(QRect(x + 10, y + 10, iconSize - 20, iconSize - 20), imgUltIcon);

    // 护盾或冻结激活时的特殊显示
    if (isShieldActive || isTimeFrozen)
    {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::green, 3));
        p.drawEllipse(x, y, iconSize, iconSize);
        p.setPen(Qt::green);
        p.drawText(x, y - 10, iconSize, 20, Qt::AlignCenter, "ACTIVE");
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
        p.drawText(x, y - 10, iconSize, 20, Qt::AlignCenter, "READY");
    }
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 10));
    p.drawText(x, y + iconSize + 5, "ULTIMATE (L-Click)");
}

void GameWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    if (!imgBg.isNull())
        p.drawImage(rect(), imgBg);
    else
        p.fillRect(rect(), Qt::black);

    // 绘制时空冻结滤镜
    if (isTimeFrozen)
    {
        p.fillRect(rect(), QColor(0, 0, 255, 50));
    }

    // 绘制激光 (仅限初始机)
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

    // 绘制英雄
    p.drawImage(heroX, heroY, imgHero);

    // 绘制护盾
    if (isShieldActive)
    {
        p.setPen(QPen(Qt::cyan, 3));
        p.setBrush(QColor(0, 255, 255, 50));
        p.drawEllipse(QPointF(heroX + imgHero.width() / 2, heroY + imgHero.height() / 2), 50, 50);
    }

    // 绘制敌人
    for (const auto &e : enemies)
    {
        if (e.type == 10)
        {
            if (currentBossFrame.isValid())
            {
                p.setCompositionMode(QPainter::CompositionMode_Screen);
                p.drawImage(QRect(e.x, e.y, 200, 150), currentBossFrame.toImage());
                p.setCompositionMode(QPainter::CompositionMode_SourceOver);
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

    // 绘制子弹
    p.setPen(Qt::NoPen);
    for (const auto &b : bullets)
    {
        if (b.isEnemy)
            p.setBrush(Qt::red);
        else
            p.setBrush(Qt::yellow);
        p.drawEllipse(b.x, b.y, 8, 8);
    }

    // UI
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(20, 40, "Level " + QString::number(currentLevelConfig.levelId));
    p.drawText(20, 70, "Score: " + QString::number(score));

    // 血条
    p.drawText(20, 100, "HP:");
    p.setBrush(Qt::gray);
    p.drawRect(60, 85, 200, 15);
    // 比例计算
    PlaneStats s = DataManager::getPlaneStats(currentPlaneId);
    float hpRatio = (float)heroHp / s.hp;
    if (hpRatio < 0)
        hpRatio = 0;

    if (hpRatio > 0.3)
        p.setBrush(QColor(0, 255, 255));
    else
        p.setBrush(Qt::red);
    p.drawRect(60, 85, 200 * hpRatio, 15);

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
        p.drawText(rect(), Qt::AlignCenter, "VICTORY!\n+Coins");
    }
}

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
        e.x = QRandomGenerator::global()->bounded(width() - 50);
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

    QString bossVideoPath = QString("assets/boss%1.mp4").arg(currentLevelConfig.levelId);
    if (!QFileInfo::exists(bossVideoPath))
        bossVideoPath = "assets/menu_bg.mp4";
    bossVideoPlayer->setSource(QUrl::fromLocalFile(bossVideoPath));
    bossVideoPlayer->setLoops(QMediaPlayer::Infinite);
    bossVideoPlayer->play();

    Enemy boss;
    boss.type = 10;
    boss.x = width() / 2 - 100;
    boss.y = -150;
    boss.active = true;
    boss.maxHp = currentLevelConfig.bossHp;
    boss.hp = boss.maxHp;
    boss.shootTimer = 0;
    boss.moveAngle = 0;
    boss.bossId = currentLevelConfig.levelId;
    enemies.append(boss);
}

void GameWidget::drawProgressBar(QPainter &p)
{
    int barWidth = 400;
    int barHeight = 20;
    int barX = (width() - barWidth) / 2;
    int barY = 10;
    p.setPen(QPen(Qt::white, 2));
    p.setBrush(QColor(0, 0, 0, 150));
    p.drawRect(barX, barY, barWidth, barHeight);
    float percentage = 0;
    if (currentLevelConfig.totalWaves > 0)
        percentage = (float)progressCounter / (float)currentLevelConfig.totalWaves;
    if (percentage > 1.0f)
        percentage = 1.0f;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(50, 205, 50));
    p.drawRect(barX + 2, barY + 2, (int)((barWidth - 4) * percentage), barHeight - 4);
    p.drawImage(barX + barWidth - 25, barY - 5, imgEnemy3.scaled(30, 30));
    if (bossSpawned)
    {
        p.setPen(Qt::red);
        p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(barX, barY + 40, barWidth, 20, Qt::AlignCenter, "BOSS INCOMING!");
    }
}

void GameWidget::gameOver()
{
    if (!isGameOver && !isVictory)
    {
        isGameOver = true;
        bossVideoPlayer->stop();
        setCursor(Qt::ArrowCursor);

        // 结算金币
        int coinsEarned = score / 10;
        DataManager::addCoins(coinsEarned);
    }
}

void GameWidget::victory()
{
    if (!isVictory && !isGameOver)
    {
        isVictory = true;
        bossVideoPlayer->stop();
        setCursor(Qt::ArrowCursor);
        ScoreManager::saveScore(score);
        LevelManager::unlockNextLevel(currentLevelConfig.levelId);

        int coinsEarned = score / 10;
        DataManager::addCoins(coinsEarned);
    }
}

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isGameOver || isVictory)
        return;
    double targetX = event->pos().x() - imgHero.width() / 2;
    double targetY = event->pos().y() - imgHero.height() / 2;
    if (targetX < 0)
        targetX = 0;
    if (targetX > width() - imgHero.width())
        targetX = width() - imgHero.width();
    if (targetY < 0)
        targetY = 0;
    if (targetY > height() - imgHero.height())
        targetY = height() - imgHero.height();
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