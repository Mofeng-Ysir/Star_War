#ifndef SCOREMANAGER_H
#define SCOREMANAGER_H

#include <QString>
#include <QList>

class ScoreManager
{
public:
    static QString getFilePath();
    static QList<int> loadScores();
    static void saveScore(int newScore);
};

#endif // SCOREMANAGER_H