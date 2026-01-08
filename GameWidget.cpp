#include "GameWidget.h"
#include "ScoreManager.h"
#include "LevelManager.h"
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

    // --- 音效 ---
    shootSfx = new QSoundEffect(this);
    shootSfx->setSource(QUrl::fromLocalFile("assets/shoot.wav"));
    shootSfx->setVolume(0.3);

    explodeSfx = new QSoundEffect(this);
    explodeSfx->setSource(QUrl::fromLocalFile("assets/explode.wav"));
    explodeSfx->setVolume(0.6);

    // 激光音效 (最好准备一个 laser.wav，没有的话就用 shoot.wav 代替)
    laserSfx = new QSoundEffect(this);
    if (QFileInfo::exists("assets/laser.wav"))
        laserSfx->setSource(QUrl::fromLocalFile("assets/laser.wav"));
    else
        laserSfx->setSource(QUrl::fromLocalFile("assets/shoot.wav"));
    laserSfx->setVolume(1.0);

    // --- 背景音乐 ---
    bgmPlayer = new QMediaPlayer(this);
    bgmOutput = new QAudioOutput(this);
    bgmPlayer->setAudioOutput(bgmOutput);
    bgmOutput->setVolume(0.3);

    // --- BOSS 视频初始化 ---
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
}

void GameWidget::loadAssets()
{
    imgHero.load("assets/hero.png");
    imgEnemy1.load("assets/enemy.png");
    imgEnemy2.load("assets/enemy2.png");
    imgEnemy3.load("assets/enemy3.png");
    imgBg.load("assets/game_bg.png");

    // 大招图标 (借用敌机2的图，如果有专门的 icon 更好)
    imgLaserIcon.load("assets/enemy2.png");

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

    heroX = width() / 2 - 30;
    heroY = height() - 100;
    score = 0;
    heroHp = 10;
    isGameOver = false;
    isVictory = false;
    heroShootTimer = 0;
    progressCounter = 0;
    bossSpawned = false;
    bossTime = 0;
    bossAttackAngle = 0;

    // 大招状态重置
    isLaserActive = false;
    laserDurationTimer = 0;
    laserCooldownTimer = LASER_COOLDOWN_MAX; // 开局满能量

    bullets.clear();
    enemies.clear();

    bossVideoPlayer->stop();

    // 播放默认关卡 BGM
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

    // 1. 切换 BOSS 专属 BGM
    // 文件名格式约定：assets/boss1_bgm.mp3
    QString bossBgmPath = QString("assets/boss%1_bgm.mp3").arg(currentLevelConfig.levelId);
    if (!QFileInfo::exists(bossBgmPath))
        bossBgmPath = "assets/game_bgm.mp3"; // 兜底

    bgmPlayer->stop();
    bgmPlayer->setSource(QUrl::fromLocalFile(bossBgmPath));
    bgmPlayer->play();

    // 2. 切换 BOSS 专属视频
    // 文件名格式约定：assets/boss1.mp4
    QString bossVideoPath = QString("assets/boss%1.mp4").arg(currentLevelConfig.levelId);
    if (!QFileInfo::exists(bossVideoPath))
        bossVideoPath = "assets/menu_bg.mp4"; // 兜底

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

// 释放激光大招
void GameWidget::fireLaser()
{
    if (isGameOver || isVictory)
        return;

    // 如果冷却好了，且当前没有在释放
    if (laserCooldownTimer >= LASER_COOLDOWN_MAX && !isLaserActive)
    {
        isLaserActive = true;
        laserDurationTimer = LASER_DURATION_MAX;
        laserCooldownTimer = 0; // 重置冷却
        laserSfx->play();

        // 震屏效果（简单的坐标偏移逻辑可以在 paintEvent 里加，这里简化）
    }
}

void GameWidget::updateGame()
{
    if (isGameOver || isVictory)
        return;

    // --- 1. 英雄自动普通射击 ---
    heroShootTimer++;
    if (heroShootTimer > 12)
    {
        heroShootTimer = 0;
        Bullet b;
        b.x = heroX + imgHero.width() / 2 - 4;
        b.y = heroY;
        b.speedX = 0;
        b.speedY = -12.0;
        b.isEnemy = false;
        b.active = true;
        bullets.append(b);
        shootSfx->play();
    }

    // --- 2. 大招逻辑更新 ---
    if (isLaserActive)
    {
        laserDurationTimer--;
        if (laserDurationTimer <= 0)
        {
            isLaserActive = false; // 大招结束
        }
    }
    else
    {
        // 冷却回复
        if (laserCooldownTimer < LASER_COOLDOWN_MAX)
        {
            laserCooldownTimer++;
        }
    }

    // --- 3. 刷怪逻辑 ---
    if (!bossSpawned)
    {
        if (progressCounter < currentLevelConfig.totalWaves)
        {
            spawnEnemy();
        }
        else
        {
            spawnBoss();
        }
    }

    // --- 4. 更新敌人与 BOSS AI ---
    for (auto &e : enemies)
    {
        if (e.type == 10)
        { // === BOSS AI ===
            bossTime += 0.02;
            bossAttackAngle += 5.0; // 旋转角度增加

            // 移动逻辑：8字形盘旋
            if (e.y < 50)
                e.y += 1.5;
            else
            {
                double targetX = (width() / 2 - 100) + 200 * qSin(bossTime * (1.0 + e.bossId * 0.1));
                double targetY = 100 + 50 * qSin(2 * bossTime);
                e.x = e.x * 0.95 + targetX * 0.05;
                e.y = e.y * 0.95 + targetY * 0.05;
            }

            e.shootTimer++;

            // === 专属攻击模式 ===
            // 基础射速
            int fireThreshold = 40;

            // Level 1: 简单的狙击弹
            if (e.bossId == 1)
            {
                if (e.shootTimer > 60)
                {
                    e.shootTimer = 0;
                    Bullet b;
                    b.x = e.x + 100;
                    b.y = e.y + 150;
                    b.isEnemy = true;
                    b.active = true;
                    // 计算追踪角度
                    double dx = (heroX + 30) - b.x;
                    double dy = (heroY + 30) - b.y;
                    double dist = qSqrt(dx * dx + dy * dy);
                    b.speedX = (dx / dist) * 8.0;
                    b.speedY = (dy / dist) * 8.0;
                    bullets.append(b);
                }
            }
            // Level 2: 三向散射 (Shotgun)
            else if (e.bossId == 2)
            {
                if (e.shootTimer > 50)
                {
                    e.shootTimer = 0;
                    for (int k = -1; k <= 1; k++)
                    {
                        Bullet b;
                        b.x = e.x + 100;
                        b.y = e.y + 150;
                        b.isEnemy = true;
                        b.active = true;
                        b.speedX = k * 3.0;
                        b.speedY = 7.0;
                        bullets.append(b);
                    }
                }
            }
            // Level 3: 螺旋弹幕 (Spiral)
            else if (e.bossId == 3)
            {
                if (e.shootTimer > 5)
                { // 极快射速
                    e.shootTimer = 0;
                    Bullet b;
                    b.x = e.x + 100;
                    b.y = e.y + 100;
                    b.isEnemy = true;
                    b.active = true;
                    // 使用 bossAttackAngle 计算旋转发射
                    double rad = qDegreesToRadians(bossAttackAngle);
                    b.speedX = qCos(rad) * 6.0;
                    b.speedY = qSin(rad) * 6.0;
                    bullets.append(b);
                }
            }
            // Level 4: 全屏随机雨 (Rain)
            else if (e.bossId == 4)
            {
                if (e.shootTimer > 10)
                {
                    e.shootTimer = 0;
                    Bullet b;
                    // X轴随机位置
                    b.x = QRandomGenerator::global()->bounded(width());
                    b.y = -20; // 从屏幕顶端落下，不一定从Boss发出来
                    b.isEnemy = true;
                    b.active = true;
                    b.speedX = 0;
                    b.speedY = 9.0 + QRandomGenerator::global()->bounded(5); // 快速下落
                    bullets.append(b);
                }
            }
            // Level 5: 综合地狱 (Helix + Tracking)
            else
            {
                if (e.shootTimer > 20)
                {
                    e.shootTimer = 0;
                    // 1. 螺旋
                    for (int k = 0; k < 4; k++)
                    {
                        Bullet b;
                        b.x = e.x + 100;
                        b.y = e.y + 100;
                        b.isEnemy = true;
                        b.active = true;
                        double rad = qDegreesToRadians(bossAttackAngle + k * 90);
                        b.speedX = qCos(rad) * 5.0;
                        b.speedY = qSin(rad) * 5.0;
                        bullets.append(b);
                    }
                    // 2. 狙击
                    Bullet b;
                    b.x = e.x + 100;
                    b.y = e.y + 150;
                    b.isEnemy = true;
                    b.active = true;
                    double dx = (heroX + 30) - b.x;
                    double dy = (heroY + 30) - b.y;
                    double dist = qSqrt(dx * dx + dy * dy);
                    b.speedX = (dx / dist) * 10.0;
                    b.speedY = (dy / dist) * 10.0;
                    bullets.append(b);
                }
            }
        }
        else
        {
            // 普通敌人 AI
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

    // 5. 更新子弹
    for (auto &b : bullets)
    {
        b.x += b.speedX;
        b.y += b.speedY;
        if (b.y < -20 || b.y > height() + 20 || b.x < -20 || b.x > width() + 20)
            b.active = false;
    }

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
    QRect heroRect(heroX + 15, heroY + 15, imgHero.width() - 30, imgHero.height() - 30);

    // 激光范围：以飞机为中心的垂直矩形
    QRect laserRect(heroX + imgHero.width() / 2 - 40, 0, 80, heroY);

    // 1. 激光判定 (如果大招开启)
    if (isLaserActive)
    {
        // 清除路径上的敌方子弹
        for (auto &b : bullets)
        {
            if (b.isEnemy && b.active)
            {
                if (laserRect.contains(b.x, b.y))
                {
                    b.active = false; // 蒸发子弹
                }
            }
        }

        // 对敌人造成巨额伤害
        for (auto &e : enemies)
        {
            if (!e.active)
                continue;
            QRect enemyRect;
            if (e.type == 10)
                enemyRect = QRect(e.x + 20, e.y + 20, 160, 110);
            else if (e.type == 2)
                enemyRect = QRect(e.x, e.y, imgEnemy3.width(), imgEnemy3.height());
            else
                enemyRect = QRect(e.x, e.y, imgEnemy1.width(), imgEnemy1.height());

            if (laserRect.intersects(enemyRect))
            {
                e.hp -= 20; // 每帧 20 伤害 (极高)

                if (e.hp <= 0)
                {
                    e.active = false;
                    explodeSfx->play();

                    if (e.type != 10 && progressCounter < currentLevelConfig.totalWaves)
                        progressCounter++;

                    int addScore = 10;
                    if (e.type == 10)
                    {
                        addScore = 500 * currentLevelConfig.levelId;
                        bossVideoPlayer->stop();
                    }
                    else if (e.type == 2)
                        addScore = 50;
                    score += addScore;
                }
            }
        }
    }

    // 2. 普通子弹判定
    for (auto &b : bullets)
    {
        if (!b.active)
            continue;
        QRect bulletRect(b.x, b.y, 8, 8);

        if (b.isEnemy)
        {
            if (heroRect.intersects(bulletRect))
            {
                b.active = false;
                heroHp--;
                if (heroHp <= 0)
                    gameOver();
            }
        }
        else
        {
            for (auto &e : enemies)
            {
                if (!e.active)
                    continue;
                QRect enemyRect;
                if (e.type == 10)
                    enemyRect = QRect(e.x + 20, e.y + 20, 160, 110);
                else if (e.type == 2)
                    enemyRect = QRect(e.x, e.y, imgEnemy3.width(), imgEnemy3.height());
                else
                    enemyRect = QRect(e.x, e.y, imgEnemy1.width(), imgEnemy1.height());

                if (enemyRect.intersects(bulletRect))
                {
                    b.active = false;
                    e.hp--;
                    if (e.hp <= 0)
                    {
                        e.active = false;
                        explodeSfx->play();

                        if (e.type != 10 && progressCounter < currentLevelConfig.totalWaves)
                            progressCounter++;

                        int addScore = 10;
                        if (e.type == 10)
                        {
                            addScore = 500 * currentLevelConfig.levelId;
                            bossVideoPlayer->stop();
                        }
                        else if (e.type == 2)
                            addScore = 50;
                        score += addScore;
                    }
                    break;
                }
            }
        }
    }

    // 3. 身体碰撞
    for (auto &e : enemies)
    {
        if (!e.active)
            continue;
        QRect enemyRect;
        if (e.type == 10)
            enemyRect = QRect(e.x + 40, e.y + 40, 120, 80);
        else if (e.type == 2)
            enemyRect = QRect(e.x, e.y, imgEnemy3.width(), imgEnemy3.height());
        else
            enemyRect = QRect(e.x, e.y, imgEnemy1.width(), imgEnemy1.height());

        if (heroRect.intersects(enemyRect))
        {
            e.active = false;
            explodeSfx->play();
            heroHp -= 3;
            if (e.type == 10)
            {
                heroHp = 0;
                bossVideoPlayer->stop();
            }
            if (heroHp <= 0)
                gameOver();
        }
    }
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
    {
        percentage = (float)progressCounter / (float)currentLevelConfig.totalWaves;
    }
    if (percentage > 1.0f)
        percentage = 1.0f;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(50, 205, 50));
    p.drawRect(barX + 2, barY + 2, (int)((barWidth - 4) * percentage), barHeight - 4);

    // 图标
    p.drawImage(barX + barWidth - 25, barY - 5, imgEnemy3.scaled(30, 30));

    if (bossSpawned)
    {
        p.setPen(Qt::red);
        p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(barX, barY + 40, barWidth, 20, Qt::AlignCenter, "BOSS INCOMING!");
    }
}

// 绘制激光大招 UI
void GameWidget::drawLaserUI(QPainter &p)
{
    int iconSize = 60;
    int x = 20;
    int y = height() - iconSize - 20;

    // 绘制冷却进度扇形
    p.setBrush(QColor(0, 0, 0, 180));
    p.setPen(QPen(Qt::white, 2));
    p.drawEllipse(x, y, iconSize, iconSize);

    // 绘制图标
    p.drawImage(QRect(x + 10, y + 10, iconSize - 20, iconSize - 20), imgLaserIcon);

    // 冷却遮罩
    if (laserCooldownTimer < LASER_COOLDOWN_MAX)
    {
        p.setBrush(QColor(0, 0, 0, 200));
        p.setPen(Qt::NoPen);
        // 倒计时角度
        int spanAngle = 360 * 16 * (LASER_COOLDOWN_MAX - laserCooldownTimer) / LASER_COOLDOWN_MAX;
        p.drawPie(x, y, iconSize, iconSize, 90 * 16, spanAngle);
    }
    else
    {
        // 就绪状态，画个发光圈
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

    // 1. 背景
    if (!imgBg.isNull())
        p.drawImage(rect(), imgBg);
    else
        p.fillRect(rect(), Qt::black);

    // 2. 绘制激光 (如果在释放中)
    if (isLaserActive)
    {
        // 创建渐变色
        QLinearGradient gradient(heroX + imgHero.width() / 2 - 40, 0, heroX + imgHero.width() / 2 + 40, 0);
        gradient.setColorAt(0.0, QColor(0, 255, 255, 0));
        gradient.setColorAt(0.2, QColor(0, 255, 255, 150));   // 青色边缘
        gradient.setColorAt(0.5, QColor(255, 255, 255, 255)); // 中心亮白
        gradient.setColorAt(0.8, QColor(0, 255, 255, 150));
        gradient.setColorAt(1.0, QColor(0, 255, 255, 0));

        p.setBrush(gradient);
        p.setPen(Qt::NoPen);
        // 画一个贯穿屏幕的矩形
        p.drawRect(heroX + imgHero.width() / 2 - 40, 0, 80, heroY);
    }

    // 3. 英雄
    p.drawImage(heroX, heroY, imgHero);

    // 4. 敌人与Boss视频
    for (const auto &e : enemies)
    {
        if (e.type == 10)
        {
            // Boss 视频帧
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
            // Boss血条
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

    // 5. 子弹
    p.setPen(Qt::NoPen);
    for (const auto &b : bullets)
    {
        if (b.isEnemy)
            p.setBrush(Qt::red);
        else
            p.setBrush(Qt::yellow);
        p.drawEllipse(b.x, b.y, 8, 8);
    }

    // 6. UI
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(20, 40, "Level " + QString::number(currentLevelConfig.levelId));
    p.drawText(20, 70, "Score: " + QString::number(score));

    // 英雄血条
    p.drawText(20, 100, "HP:");
    p.setBrush(Qt::gray);
    p.drawRect(60, 85, 200, 15);
    if (heroHp > 3)
        p.setBrush(QColor(0, 255, 255));
    else
        p.setBrush(Qt::red);
    int hpWidth = heroHp * 20;
    if (hpWidth < 0)
        hpWidth = 0;
    p.drawRect(60, 85, hpWidth, 15);

    drawProgressBar(p);

    // 绘制大招 UI
    drawLaserUI(p);

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

void GameWidget::gameOver()
{
    if (!isGameOver && !isVictory)
    {
        isGameOver = true;
        bossVideoPlayer->stop();
        setCursor(Qt::ArrowCursor);
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

// 鼠标点击释放大招 (原先的子弹是自动发射的，这里左键用来放激光)
void GameWidget::mousePressEvent(QMouseEvent *event)
{
    if (isGameOver || isVictory)
        return;

    if (event->button() == Qt::LeftButton)
    {
        fireLaser();
    }
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (isGameOver)
    {
        emit gameEnded();
    }
    else if (isVictory)
    {
        emit levelWon();
    }
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