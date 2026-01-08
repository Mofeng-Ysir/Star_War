#include "GameWidget.h"
#include "ScoreManager.h"
#include "LevelManager.h"
#include "CollisionSystem.h" // 引入碰撞系统
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

    laserSfx = new QSoundEffect(this);
    if (QFileInfo::exists("assets/laser.wav"))
        laserSfx->setSource(QUrl::fromLocalFile("assets/laser.wav"));
    else
        laserSfx->setSource(QUrl::fromLocalFile("assets/shoot.wav"));
    laserSfx->setVolume(1.0);

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
}

void GameWidget::loadAssets()
{
    imgHero.load("assets/hero.png");
    imgEnemy1.load("assets/enemy.png");
    imgEnemy2.load("assets/enemy2.png");
    imgEnemy3.load("assets/enemy3.png");
    imgBg.load("assets/game_bg.png");
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
    enemySpawnTimer = 0;

    // 重置大招和Boss策略
    isLaserActive = false;
    laserDurationTimer = 0;
    laserCooldownTimer = LASER_COOLDOWN_MAX;
    bossStrategy.reset(); // 重要

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

void GameWidget::fireLaser()
{
    if (isGameOver || isVictory)
        return;
    if (laserCooldownTimer >= LASER_COOLDOWN_MAX && !isLaserActive)
    {
        isLaserActive = true;
        laserDurationTimer = LASER_DURATION_MAX;
        laserCooldownTimer = 0;
        laserSfx->play();
    }
}

void GameWidget::updateGame()
{
    if (isGameOver || isVictory)
        return;

    // 1. 英雄自动射击
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

    // 2. 大招逻辑
    if (isLaserActive)
    {
        laserDurationTimer--;
        if (laserDurationTimer <= 0)
            isLaserActive = false;
    }
    else
    {
        if (laserCooldownTimer < LASER_COOLDOWN_MAX)
            laserCooldownTimer++;
    }

    // 3. 刷怪逻辑
    if (!bossSpawned)
    {
        if (progressCounter < currentLevelConfig.totalWaves)
            spawnEnemy();
        else
            spawnBoss();
    }

    // 4. 敌人更新
    for (auto &e : enemies)
    {
        if (e.type == 10)
        {
            // === 核心修改：委托给 BossStrategy 处理 ===
            bossStrategy.update(e, bullets, heroX, heroY, width(), height(), currentLevelConfig);
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

    // 6. 碰撞检测 (委托给 CollisionSystem)
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
    auto result = CollisionSystem::check(
        heroX, heroY, imgHero.width(), imgHero.height(),
        bullets, enemies, isLaserActive,
        currentLevelConfig.levelId, currentLevelConfig.totalWaves,
        progressCounter, imgEnemy1, imgEnemy3);

    // 处理结果
    score += result.scoreAdded;
    if (result.scoreAdded > 0)
    {
        // 如果有得分，说明有击杀，可能需要播放音效
        // 为了简单，这里如果得分了就播爆炸声，可能有些频繁，但反馈感好
        // 如果想更精细，CollisionSystem 可以返回 bool enemyDied
        explodeSfx->play();
    }

    if (result.bossDied)
    {
        bossVideoPlayer->stop();
    }

    if (result.heroHit)
    {
        heroHp -= result.heroDamageTaken;
        if (heroHp <= 0)
            gameOver();
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

void GameWidget::drawLaserUI(QPainter &p)
{
    int iconSize = 60;
    int x = 20;
    int y = height() - iconSize - 20;
    p.setBrush(QColor(0, 0, 0, 180));
    p.setPen(QPen(Qt::white, 2));
    p.drawEllipse(x, y, iconSize, iconSize);
    p.drawImage(QRect(x + 10, y + 10, iconSize - 20, iconSize - 20), imgLaserIcon);
    if (laserCooldownTimer < LASER_COOLDOWN_MAX)
    {
        p.setBrush(QColor(0, 0, 0, 200));
        p.setPen(Qt::NoPen);
        int spanAngle = 360 * 16 * (LASER_COOLDOWN_MAX - laserCooldownTimer) / LASER_COOLDOWN_MAX;
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

    if (isLaserActive)
    {
        QLinearGradient gradient(heroX + imgHero.width() / 2 - 40, 0, heroX + imgHero.width() / 2 + 40, 0);
        gradient.setColorAt(0.0, QColor(0, 255, 255, 0));
        gradient.setColorAt(0.2, QColor(0, 255, 255, 150));
        gradient.setColorAt(0.5, QColor(255, 255, 255, 255));
        gradient.setColorAt(0.8, QColor(0, 255, 255, 150));
        gradient.setColorAt(1.0, QColor(0, 255, 255, 0));
        p.setBrush(gradient);
        p.setPen(Qt::NoPen);
        p.drawRect(heroX + imgHero.width() / 2 - 40, 0, 80, heroY);
    }

    p.drawImage(heroX, heroY, imgHero);
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

    p.setPen(Qt::NoPen);
    for (const auto &b : bullets)
    {
        if (b.isEnemy)
            p.setBrush(Qt::red);
        else
            p.setBrush(Qt::yellow);
        p.drawEllipse(b.x, b.y, 8, 8);
    }

    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(20, 40, "Level " + QString::number(currentLevelConfig.levelId));
    p.drawText(20, 70, "Score: " + QString::number(score));

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