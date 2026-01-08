#ifndef HIGHSCOREWIDGET_H
#define HIGHSCOREWIDGET_H

#include <QWidget>
#include <QImage>
#include <QLabel>
#include <QList>
#include <QScrollArea>

class HighScoreWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HighScoreWidget(QWidget *parent = nullptr);
    void refreshScores();

signals:
    void backClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage bgImg;
    QWidget *scrollContent;
    QList<QLabel *> scoreLabels;
};

#endif // HIGHSCOREWIDGET_H