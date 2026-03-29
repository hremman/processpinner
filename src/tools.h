#ifndef TOOLS_H
#define TOOLS_H

#include <QAbstractButton>
#include <QString>


namespace tools
{
    QString buildAbsolutePath(const QString & app, const QString &project_dir, const QString &cwd);
    std::pair<QString, QStringList>  selectExecutor(const QString & app);
    void buildToolTip(const QString&, QAbstractButton *,  QAction* = nullptr);
    QString tipShortcut(const QKeySequence &ks);
}

#endif // TOOLS_H
