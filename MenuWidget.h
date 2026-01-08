#ifndef MENUWIDGET_H
#define MENUWIDGET_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoSink>
#include <QVideoFrame>

class MenuWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MenuWidget(QWidget *parent = nullptr);
    void startMenu();
    void stopMenu();

signals:
    void startClicked();
    void historyClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QMediaPlayer *player;
    QMediaPlayer *menuBgmPlayer;
    QVideoSink *videoSink;
    QVideoFrame currentVideoFrame;
    QAudioOutput *audioOutput;
    QAudioOutput *menuBgmOutput;
};

#endif // MENUWIDGET_H