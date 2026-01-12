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

// 构造函数
GameWidget::GameWidget(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    loadAssets();

    // 音效初始化
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

    // BGM 初始化
    bgmPlayer = new QMediaPlayer(this);
    bgmOutput = new QAudioOutput(this);
    bgmPlayer->setAudioOutput(bgmOutput);
    bgmOutput->setVolume(0.3);

    // --- 【核心修改】GIF 初始化 ---
    bossMovie = new QMovie(this);
    bossMovie->setCacheMode(QMovie::CacheAll); // 缓存所有帧以提高性能

    // 定时器
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &GameWidget::updateGame);

    // 变量初始化
    enemySpawnTimer = 0;
    isShieldActive = false;
    isTimeFrozen = false;
    isUltActive = false;
}

// 资源加载
void GameWidget::loadAssets()
{
    imgHero.load("assets/hero.png");
    imgEnemy1.load("assets/enemy.png");
    imgEnemy2.load("assets/enemy2.png");
    imgEnemy3.load("assets/enemy3.png"); // 用于静态图兜底
    imgBg.load("assets/game_bg.png");
    imgUltIcon.load("assets/enemy2.png");

    if (!imgHero.isNull())
        imgHero = imgHero.scaled(60, 60, Qt::KeepAspectRatio);
    if (!imgEnemy1.isNull())
        imgEnemy1 = imgEnemy1.scaled(50, 50, Qt::KeepAspectRatio);
    if (!imgEnemy2.isNull())
        imgEnemy2 = imgEnemy2.scaled(50, 50, Qt::KeepAspectRatio);
    if (!imgEnemy3.isNull())
        imgEnemy3 = imgEnemy3.scaled(120, 120, Qt::KeepAspectRatio);
}

// 分辨率适配计算
void GameWidget::getScaleOffset(double &scale, double &offsetX, double &offsetY)
{
    double w = width();
    double h = height();
    double scaleX = w / LOGICAL_WIDTH;
    double scaleY = h / LOGICAL_HEIGHT;
    scale = qMin(scaleX, scaleY);
    offsetX = (w - LOGICAL_WIDTH * scale) / 2.0;
    offsetY = (h - LOGICAL_HEIGHT * scale) / 2.0;
}

QPointF GameWidget::mapToGame(const QPoint &pos)
{
    double scale, dx, dy;
    getScaleOffset(scale, dx, dy);
    double x = (pos.x() - dx) / scale;
    double y = (pos.y() - dy) / scale;
    return QPointF(x, y);
}

// 开始游戏
void GameWidget::startGame(int level)
{
    setCursor(Qt::BlankCursor);
    currentLevelConfig = LevelManager::getLevelConfig(level);

    // 加载飞机数据
    currentPlaneId = DataManager::getCurrentPlaneId();
    PlaneStats stats = DataManager::getPlaneStats(currentPlaneId);

    heroX = LOGICAL_WIDTH / 2 - 30;
    heroY = LOGICAL_HEIGHT - 100;
    score = 0;
    heroHp = stats.hp;

    // 换皮肤
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

    // 重置状态
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

    bossStrategy.reset();

    bullets.clear();
    enemies.clear();

    // 停止上一局的动画
    if (bossMovie->isValid())
        bossMovie->stop();

    // 播放BGM
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
        bossMovie->stop(); // 停止动画
    setCursor(Qt::ArrowCursor);
}

// 释放大招
void GameWidget::fireUlt()
{
    if (isGameOver || isVictory)
        return;

    if (ultCooldownTimer >= ULT_COOLDOWN_MAX && !isUltActive && !isShieldActive && !isTimeFrozen)
    {
        ultCooldownTimer = 0;

        switch (currentPlaneId)
        {
        case PLANE_DEFAULT:
            isUltActive = true;
            ultDurationTimer = 20;
            ultSfx->play();
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
            ultSfx->play();
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
                        score += 100;
                        if (e.type == 10 && bossMovie->isValid())
                            bossMovie->stop();
                    }
                }
            }
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

// 游戏主循环
void GameWidget::updateGame()
{
    if (isGameOver || isVictory)
        return;

    // 1. 英雄普攻
    heroShootTimer++;
    int shootInterval = 12;
    if (currentPlaneId == PLANE_SNIPER)
        shootInterval = 8;

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
            // BOSS 策略移动
            bossStrategy.update(e, bullets, heroX, heroY, LOGICAL_WIDTH, LOGICAL_HEIGHT, currentLevelConfig);
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
                double d = qSqrt(qPow(e.x - b.x, 2) + qPow(e.y - b.y, 2));
                if (d < minDist)
                {
                    minDist = d;
                    closest = &e;
                }
            }
            if (closest)
            {
                if (closest->x > b.x)
                    b.speedX += 0.5;
                else
                    b.speedX -= 0.5;
                if (b.speedX > 5)
                    b.speedX = 5;
                if (b.speedX < -5)
                    b.speedX = -5;
            }
        }

        b.x += b.speedX;
        b.y += b.speedY;
        if (b.y < -20 || b.y > LOGICAL_HEIGHT + 20 || b.x < -20 || b.x > LOGICAL_WIDTH + 20)
            b.active = false;
    }

    // 6. 碰撞检测
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
        currentLevelConfig.levelId, currentLevelConfig.totalWaves,
        progressCounter, imgEnemy1, imgEnemy3);

    score += result.scoreAdded;
    if (result.scoreAdded > 0)
        explodeSfx->play();

    if (result.bossDied)
    {
        if (bossMovie->isValid())
            bossMovie->stop(); // Boss死了停止动画
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

// 刷怪辅助
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
        e.x = QRandomGenerator::global()->bounded(LOGICAL_WIDTH - 50);
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

// 生成BOSS
void GameWidget::spawnBoss()
{
    if (bossSpawned)
        return;
    bossSpawned = true;

    // 1. BGM
    QString bossBgmPath = QString("assets/boss%1_bgm.mp3").arg(currentLevelConfig.levelId);
    if (!QFileInfo::exists(bossBgmPath))
        bossBgmPath = "assets/game_bgm.mp3";
    bgmPlayer->stop();
    bgmPlayer->setSource(QUrl::fromLocalFile(bossBgmPath));
    bgmPlayer->play();

    // 2. 【核心修改】加载 GIF
    // 如果没有 gif，使用静态图片显示
    QString bossGifPath = QString("assets/boss%1.gif").arg(currentLevelConfig.levelId);
    if (QFileInfo::exists(bossGifPath))
    {
        bossMovie->setFileName(bossGifPath);
        bossMovie->start(); // 开始播放
    }
    else
    {
        bossMovie->setFileName(""); // 清空
    }

    Enemy boss;
    boss.type = 10;
    boss.x = LOGICAL_WIDTH / 2 - 100;
    boss.y = -150;
    boss.active = true;
    boss.maxHp = currentLevelConfig.bossHp;
    boss.hp = boss.maxHp;
    boss.shootTimer = 0;
    boss.moveAngle = 0;
    boss.bossId = currentLevelConfig.levelId;
    enemies.append(boss);
}

// === 绘制部分 ===
void GameWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    // 1. 全屏黑底
    p.fillRect(rect(), Qt::black);

    // 2. 缩放
    double scale, dx, dy;
    getScaleOffset(scale, dx, dy);
    p.translate(dx, dy);
    p.scale(scale, scale);
    p.setClipRect(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT);

    // 3. 绘制内容
    if (!imgBg.isNull())
        p.drawImage(QRect(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT), imgBg);
    else
        p.fillRect(QRect(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT), Qt::black);

    // 3.1 时空冻结
    if (isTimeFrozen)
        p.fillRect(QRect(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT), QColor(0, 0, 255, 50));

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
            // 【核心修改】绘制 GIF 当前帧
            if (bossMovie->isValid() && bossMovie->state() == QMovie::Running)
            {
                // 绘制大小：200x150 (可根据gif实际比例调整)
                p.drawPixmap(QRect(e.x, e.y, 200, 150), bossMovie->currentPixmap());
            }
            else
            {
                // GIF不存在则画静态图
                p.drawImage(e.x, e.y, imgEnemy3);
            }

            // 血条
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
        if (b.isEnemy)
            p.setBrush(Qt::red);
        else
            p.setBrush(Qt::yellow);
        p.drawEllipse(b.x, b.y, 8, 8);
    }

    // 3.6 UI
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(20, 40, "Level " + QString::number(currentLevelConfig.levelId));
    p.drawText(20, 70, "Score: " + QString::number(score));

    p.drawText(20, 100, "HP:");
    p.setBrush(Qt::gray);
    p.drawRect(60, 85, 200, 15);
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
        p.drawText(QRect(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT), Qt::AlignCenter, "GAME OVER");
    }
    else if (isVictory)
    {
        p.setPen(Qt::yellow);
        p.setFont(QFont("Microsoft YaHei", 40, QFont::Bold));
        p.drawText(QRect(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT), Qt::AlignCenter, "VICTORY!\nLevel Complete");
        p.setFont(QFont("Arial", 20));
        p.drawText(QRect(0, 150, LOGICAL_WIDTH, LOGICAL_HEIGHT), Qt::AlignCenter, "Click to Continue");
    }
}

// UI 绘制辅助
void GameWidget::drawProgressBar(QPainter &p)
{
    int barWidth = 400;
    int barHeight = 20;
    int barX = (LOGICAL_WIDTH - barWidth) / 2;
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

void GameWidget::drawUltUI(QPainter &p)
{
    int iconSize = 60;
    int x = 20;
    int y = LOGICAL_HEIGHT - iconSize - 20;
    p.setBrush(QColor(0, 0, 0, 180));
    p.setPen(QPen(Qt::white, 2));
    p.drawEllipse(x, y, iconSize, iconSize);
    p.drawImage(QRect(x + 10, y + 10, iconSize - 20, iconSize - 20), imgUltIcon);

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

// 结算逻辑
void GameWidget::gameOver()
{
    if (!isGameOver && !isVictory)
    {
        isGameOver = true;
        if (bossMovie->isValid())
            bossMovie->stop();
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
            bossMovie->stop();
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

    if (targetX < 0)
        targetX = 0;
    if (targetX > LOGICAL_WIDTH - imgHero.width())
        targetX = LOGICAL_WIDTH - imgHero.width();
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