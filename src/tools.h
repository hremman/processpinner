#ifndef TOOLS_H
#define TOOLS_H

#include <QString>


namespace tools
{
    QString buildAbsolutePath(const QString & app, const QString &project_dir, const QString &cwd);
    std::pair<QString, QStringList>  selectExecutor(const QString & app);
}

#endif // TOOLS_H
