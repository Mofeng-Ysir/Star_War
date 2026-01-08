#include "ScoreManager.h"
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <algorithm>

QString ScoreManager::getFilePath()
{
    return QCoreApplication::applicationDirPath() + "/scores.txt";
}

QList<int> ScoreManager::loadScores()
{
    QList<int> scores;
    QFile file(getFilePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            bool ok;
            int score = line.toInt(&ok);
            if (ok && score > 0)
                scores.append(score);
        }
        file.close();
    }
    std::sort(scores.begin(), scores.end(), std::greater<int>());
    return scores;
}

void ScoreManager::saveScore(int newScore)
{
    if (newScore <= 0)
        return;
    QList<int> scores = loadScores();
    scores.append(newScore);
    std::sort(scores.begin(), scores.end(), std::greater<int>());

    while (scores.size() > 10)
        scores.removeLast();

    QFile file(getFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        for (int s : scores)
        {
            out << s << "\n";
        }
        file.close();
    }
}