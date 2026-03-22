#ifndef JAVAEXECUTOR_H
#define JAVAEXECUTOR_H

#include <QFileInfo>
#include <vector>


class JavaExecutor
{
public:
    static bool detector(const QFileInfo & file);
    static QString runString(const QFileInfo & file);
    static const QString extension;
    static const std::pair<QString, QString (*)(const QFileInfo &)> pair;
};

#endif // JAVAEXECUTOR_H
