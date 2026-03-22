#ifndef PYTHONEXECUTOR_H
#define PYTHONEXECUTOR_H

#include <QFileInfo>
#include <vector>


class PythonExecutor
{
public:
    static bool detector(const QFileInfo & file);
    static QString runString(const QFileInfo & file);
    static const QString extension;
    static const std::pair<QString, QString (*)(const QFileInfo &)> pair;
    static QStringList envs(const QFileInfo &);
private:
    static const std::vector<QString> venvs;
};

#endif // PYTHONEXECUTOR_H
